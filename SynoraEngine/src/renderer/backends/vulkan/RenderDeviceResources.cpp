#include "Limits.h"
#include "RenderDevice.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "core/Buffer.h"
#include "core/Commands.h"
#include "core/Device.h"
#include "core/Image.h"
#include "core/Pipeline.h"
#include "core/StagingBuffer.h"

#include <GLFW/glfw3.h>
#include <cstring>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <vk_mem_alloc.h>

#include <SynoraEngine/core/Window.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

BufferHandle SYN::VK::VulkanRenderDevice::createBuffer(const BufferDesc &desc) {
    Buffer buffer{
        VK::createBuffer(m_Device, m_Allocator, desc.size,
                         VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                             VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         0)};

    BufferHandle handle{m_Buffers.insert(buffer)};

    return handle;
}

void SYN::VK::VulkanRenderDevice::destroyBuffer(BufferHandle handle) {
    vkDeviceWaitIdle(m_Device.logical);

    Buffer buffer{m_Buffers[handle]};

    VK::destroyBuffer(m_Allocator, buffer);

    m_Buffers.remove(handle);

    handle.id = UINT32_MAX;
}

TextureHandle
SYN::VK::VulkanRenderDevice::createTexture(const TextureDesc &desc) {
    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};
    VkImageAspectFlags aspect{};
    VkImageUsageFlags usage{VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT};
    if (desc.hasMipChain) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    switch (desc.type) {
    case TextureType::srgb:
        format = VK_FORMAT_R8G8B8A8_SRGB;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    case TextureType::depth:
        format = VK_FORMAT_D32_SFLOAT;
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
    case TextureType::rgba:
        format = VK_FORMAT_R8G8B8A8_UNORM;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    default:
        spdlog::warn("Could not add texture, invalid format");
        break;
    }

    uint32_t mipLevels{1};
    if (desc.hasMipChain) {
        mipLevels = static_cast<uint32_t>(
            std::floor(std::log2(std::max(desc.height, desc.width))) + 1);
    }

    uint32_t layerCount{1};
    if (desc.isCubeMap) {
        layerCount = 6;
    }

    Image image{createImage(m_Device, m_Allocator, format,
                            {.width = desc.width, .height = desc.height}, usage,
                            aspect, VK_SAMPLE_COUNT_1_BIT, mipLevels,
                            layerCount, desc.isCubeMap)};

    if (m_BindlessTextureIndexFreelist.empty() && !desc.isCubeMap) {
        spdlog::warn("Could not create attachment, bindless array is full");
        destroyImage(m_Device, m_Allocator, image);
        return {};
    }

    VkDescriptorImageInfo imageInfo{
        .imageView = image.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    uint32_t descriptorElement{m_BindlessTextureIndexFreelist.back()};
    m_BindlessTextureIndexFreelist.pop_back();

    VkWriteDescriptorSet bindlessWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_BindlessDescriptorSet,
        .dstBinding = Limits::c_TextureBinding,
        .dstArrayElement = descriptorElement,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &imageInfo,
    };

    vkUpdateDescriptorSets(m_Device.logical, 1, &bindlessWrite, 0, nullptr);

    Texture texture{.image = std::move(image),
                    .bindlessSamplerIndex = descriptorElement};

    TextureHandle handle{m_Textures.insert(std::move(texture))};
    return handle;
}

void SYN::VK::VulkanRenderDevice::destroyTexture(TextureHandle handle) {
    vkDeviceWaitIdle(m_Device.logical);

    Texture texture{m_Textures[handle]};

    destroyImage(m_Device, m_Allocator, texture.image);

    m_BindlessTextureIndexFreelist.emplace_back(texture.bindlessSamplerIndex);

    m_Textures.remove(handle);
    handle.id = UINT32_MAX;
}

AttachmentHandle
SYN::VK::VulkanRenderDevice::createAttachment(const AttachmentDesc &desc) {
    VkFormat format{};
    VkImageAspectFlags aspect{};
    VkImageUsageFlags usage{VK_IMAGE_USAGE_SAMPLED_BIT};
    switch (desc.type) {
    case TextureType::srgb:
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        format = m_Swapchain.format;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    case TextureType::depth:
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        format = VK_FORMAT_D32_SFLOAT;
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
    case TextureType::rgba:
        spdlog::warn("Trying to use rgba texture type for attachment, all "
                     "attachments should be srgb");
        format = m_Swapchain.format;
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    default:
        spdlog::warn("Could not add texture, invalid format");
        return {};
        break;
    }

    uint32_t width{desc.width};
    uint32_t height{desc.height};
    if (desc.size == AttachmentSize::relative) {
        width = m_Swapchain.extent.width;
        height = m_Swapchain.extent.height;
    }

    Image image{};
    uint32_t bindlessSamplerIndex{};
    VkSampleCountFlagBits samples{getSamples(m_Device, desc.msaaSamples)};

    uint32_t layerCount{1};
    if (desc.isCubeMap) {
        layerCount = 6;
    }

    image = createImage(m_Device, m_Allocator, format,
                        {.width = width, .height = height}, usage, aspect,
                        samples, 1, layerCount, desc.isCubeMap);

    if (m_BindlessTextureIndexFreelist.empty() && !desc.isCubeMap) {
        spdlog::warn("Could not create attachment, bindless array is full");

        destroyImage(m_Device, m_Allocator, image);

        return {};
    }

    VkDescriptorImageInfo imageInfo{
        .imageView = image.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    uint32_t descriptorElement{m_BindlessTextureIndexFreelist.back()};
    m_BindlessTextureIndexFreelist.pop_back();

    VkWriteDescriptorSet bindlessWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_BindlessDescriptorSet,
        .dstBinding = Limits::c_TextureBinding,
        .dstArrayElement = descriptorElement,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &imageInfo,
    };

    vkUpdateDescriptorSets(m_Device.logical, 1, &bindlessWrite, 0, nullptr);
    bindlessSamplerIndex = descriptorElement;

    Attachment attachment{
        .image = image,
        .size = desc.size,
        .bindlessSamplerIndex = bindlessSamplerIndex,
    };

    return m_Attachments.insert(attachment);
}

void SYN::VK::VulkanRenderDevice::destroyAttachment(AttachmentHandle &handle) {
    vkDeviceWaitIdle(m_Device.logical);

    Attachment &attachment{m_Attachments[handle]};
    destroyImage(m_Device, m_Allocator, attachment.image);

    m_BindlessTextureIndexFreelist.emplace_back(
        attachment.bindlessSamplerIndex);

    m_Attachments.remove(handle);
    handle.id = UINT32_MAX;
}

PipelineHandle
SYN::VK::VulkanRenderDevice::createPipeline(const GraphicsPipelineDesc &desc) {
    GraphicsPipelineBuilder pipelineBuilder{};
    pipelineBuilder.setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    if (desc.hasDepthTesting) {
        pipelineBuilder.enableDepthTesting();
    }
    if (desc.hasDepthWriting) {
        pipelineBuilder.enableDepthWriting();
    }

    for (size_t i{}; i < desc.nColorAttachments; i++) {
        pipelineBuilder.addColorAttachment(m_Swapchain.format,
                                           desc.hasAlphaBlending);
    }

    switch (desc.cullMode) {
    case CullMode::frontFace:
        pipelineBuilder.setFaceCulling(VK_CULL_MODE_FRONT_BIT,
                                       VK_FRONT_FACE_CLOCKWISE);
        break;
    case CullMode::backFace:
        pipelineBuilder.setFaceCulling(VK_CULL_MODE_BACK_BIT,
                                       VK_FRONT_FACE_CLOCKWISE);
        break;
    default:
        break;
    }

    switch (desc.polygonMode) {
    case PolygonMode::fill:
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_FILL);
        break;
    case PolygonMode::line:
        pipelineBuilder.setPolygonMode(VK_POLYGON_MODE_LINE);
    default:
        break;
    }

    VkSampleCountFlagBits sampleCount{getSamples(m_Device, desc.msaaSamples)};
    pipelineBuilder.setMSAA(sampleCount);

    std::unordered_map<VkShaderStageFlagBits, std::string> shaderPaths{};

    if (desc.vertexShaderPath.has_value()) {
        shaderPaths[VK_SHADER_STAGE_VERTEX_BIT] = desc.vertexShaderPath.value();
    }
    if (desc.fragmentShaderPath.has_value()) {
        shaderPaths[VK_SHADER_STAGE_FRAGMENT_BIT] =
            desc.fragmentShaderPath.value();
    }

    VkPipeline pipeline{
        pipelineBuilder.build(m_Device, m_GraphicsPipelineLayout, shaderPaths)};

    return m_Pipelines.insert(pipeline);
}

void SYN::VK::VulkanRenderDevice::destroyPipeline(PipelineHandle &handle) {
    vkDeviceWaitIdle(m_Device.logical);

    VkPipeline pipeline{m_Pipelines[handle]};

    vkDestroyPipeline(m_Device.logical, pipeline, nullptr);

    m_Pipelines.remove(handle);
    handle.id = UINT32_MAX;
}
