#include "../core/Commands.h"
#include "../core/Pipeline.h"
#include "RenderDevice.h"
#include "SynoraEngine/renderer/RenderTypes.h"

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

namespace {
VkImageAspectFlags getVkAspect(TextureType type);
VkFormat getVkFormat(TextureFormat format, TextureType type);
} // namespace

BufferHandle SYN::RenderDevice::createBuffer(const BufferDesc &desc) {
    Buffer buffer{
        VK::createBuffer(m_Device, m_Allocator, desc.size,
                         VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                             VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         0)};

    BufferHandle handle{m_Buffers.insert(buffer)};

    return handle;
}

void SYN::RenderDevice::destroyBuffer(BufferHandle &handle) {
    vkDeviceWaitIdle(m_Device.logical);

    Buffer buffer{m_Buffers[handle]};

    VK::destroyBuffer(m_Allocator, buffer);

    m_Buffers.remove(handle);

    handle.id = UINT32_MAX;
}

TextureHandle SYN::RenderDevice::createTexture(const TextureDesc &desc) {
    VkFormat format{getVkFormat(desc.format, desc.type)};
    VkImageAspectFlags aspect{getVkAspect(desc.type)};
    VkImageUsageFlags usage{VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT};
    if (desc.hasMipChain) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
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
        .dstSet = m_BindlessSet,
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

void SYN::RenderDevice::destroyTexture(TextureHandle &handle) {
    vkDeviceWaitIdle(m_Device.logical);

    Texture texture{m_Textures[handle]};

    destroyImage(m_Device, m_Allocator, texture.image);

    m_BindlessTextureIndexFreelist.emplace_back(texture.bindlessSamplerIndex);

    m_Textures.remove(handle);
    handle.id = UINT32_MAX;
}

AttachmentHandle
SYN::RenderDevice::createAttachment(const AttachmentDesc &desc) {
    if (desc.mipLevels > 1) {
        assert(desc.isStorageImage);
    }

    VkFormat format{getVkFormat(desc.format, desc.type)};
    VkImageAspectFlags aspect{getVkAspect(desc.type)};
    VkImageUsageFlags usage{VK_IMAGE_USAGE_SAMPLED_BIT};

    if (aspect == VK_IMAGE_ASPECT_COLOR_BIT) {
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    } else {
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    if (desc.isStorageImage) {
        assert(desc.isCubeMap == false);
        assert(desc.msaaSamples == 1);
        assert(!(usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT));

        if (desc.msaaSamples != 1 || desc.isCubeMap != false ||
            (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
            spdlog::warn("Trying to create an attachment that is a storage "
                         "image that conflicts with msaa samples, isCubemap, "
                         "or image aspect; not enabling storage image for this "
                         "attachment");
        } else {
            usage |= VK_IMAGE_USAGE_STORAGE_BIT;
        }
    }

    uint32_t width{desc.width};
    uint32_t height{desc.height};
    if (desc.size == AttachmentSize::relative) {
        width = m_Swapchain.extent.width;
        height = m_Swapchain.extent.height;
    }

    VkSampleCountFlagBits samples{getSamples(m_Device, desc.msaaSamples)};

    uint32_t layerCount{1};
    if (desc.isCubeMap) {
        layerCount = 6;
    }

    Image image{};
    image = createImage(m_Device, m_Allocator, format,
                        {.width = width, .height = height}, usage, aspect,
                        samples, desc.mipLevels, layerCount, desc.isCubeMap);

    if (m_BindlessTextureIndexFreelist.empty() && !desc.isCubeMap) {
        spdlog::warn("Could not create attachment, bindless array is full");

        destroyImage(m_Device, m_Allocator, image);

        return {};
    }

    std::vector<VkWriteDescriptorSet> bindlessWrites{};

    VkDescriptorImageInfo textureImageInfo{
        .imageView = image.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    uint32_t textureElement{m_BindlessTextureIndexFreelist.back()};
    m_BindlessTextureIndexFreelist.pop_back();

    VkWriteDescriptorSet textureWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_BindlessSet,
        .dstBinding = Limits::c_TextureBinding,
        .dstArrayElement = textureElement,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &textureImageInfo,
    };
    bindlessWrites.emplace_back(textureWrite);

    uint32_t bindlessTextureIndex{textureElement};
    std::vector<uint32_t> bindlessStorageImageIndices{};

    std::vector<VkDescriptorImageInfo> storageImageInfos(image.mipViews.size());
    if (desc.isStorageImage) {
        assert(desc.mipLevels == image.mipViews.size());

        for (size_t i{}; i < desc.mipLevels; i++) {
            storageImageInfos[i] = {
                .imageView = image.mipViews[i],
                .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
            };

            // TODO: refactor this whole function, getting long, also make this
            // auto free all the attachments created here
            if (m_BindlessStorageImageFreelist.empty()) {
                spdlog::error("Bindless storage images all used up ):");
                assert(false);
            }

            uint32_t storageImageElement{m_BindlessStorageImageFreelist.back()};
            m_BindlessStorageImageFreelist.pop_back();

            VkWriteDescriptorSet storageImageWrite{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_BindlessSet,
                .dstBinding = Limits::c_StorageImageBinding,
                .dstArrayElement = storageImageElement,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &storageImageInfos[i],
            };

            bindlessWrites.emplace_back(storageImageWrite);
            bindlessStorageImageIndices.emplace_back(storageImageElement);
        }
    }

    vkUpdateDescriptorSets(m_Device.logical, bindlessWrites.size(),
                           bindlessWrites.data(), 0, nullptr);

    Attachment attachment{
        .image = image,
        .size = desc.size,
        .bindlessTextureIndex = bindlessTextureIndex,
        .bindlessStorageImageIndices = bindlessStorageImageIndices,
    };

    return m_Attachments.insert(attachment);
}

void SYN::RenderDevice::destroyAttachment(AttachmentHandle &handle) {
    vkDeviceWaitIdle(m_Device.logical);

    Attachment &attachment{m_Attachments[handle]};
    destroyImage(m_Device, m_Allocator, attachment.image);

    m_BindlessTextureIndexFreelist.emplace_back(
        attachment.bindlessTextureIndex);

    for (auto index : attachment.bindlessStorageImageIndices) {
        m_BindlessStorageImageFreelist.emplace_back(index);
    }

    m_Attachments.remove(handle);
    handle.id = UINT32_MAX;
}

PipelineHandle
SYN::RenderDevice::createPipeline(const GraphicsPipelineDesc &desc) {
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
        pipelineBuilder.build(m_Device, m_PipelineLayout, shaderPaths)};

    return m_Pipelines.insert(pipeline);
}

PipelineHandle
SYN::RenderDevice::createPipeline(const ComputePipelineDesc &desc) {
    VkPipeline pipeline{buildComputePipeline(m_Device, m_PipelineLayout,
                                             std::string(desc.shaderPath))};

    return m_Pipelines.insert(pipeline);
}

void SYN::RenderDevice::destroyPipeline(PipelineHandle &handle) {
    vkDeviceWaitIdle(m_Device.logical);

    VkPipeline pipeline{m_Pipelines[handle]};

    vkDestroyPipeline(m_Device.logical, pipeline, nullptr);

    m_Pipelines.remove(handle);
    handle.id = UINT32_MAX;
}

VkCommandBuffer SYN::RenderDevice::acquireCommandBuffer() {
    pollSubmissions();

    VkCommandBuffer cmdBuffer{};
    if (m_FreeCmdBuffers.empty()) {
        VkCommandBufferAllocateInfo allocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_TransientCmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkResult res{
            vkAllocateCommandBuffers(m_Device.logical, &allocInfo, &cmdBuffer)};
        if (res != VK_SUCCESS) {
            spdlog::error(
                "Could not create Vulkan Command Buffer. VkResult = {}",
                static_cast<int>(res));
        }
    } else {
        cmdBuffer = m_FreeCmdBuffers.back();
        m_FreeCmdBuffers.pop_back();
    }

    return cmdBuffer;
}

bool SYN::RenderDevice::getReceiptStatus(Receipt receipt) {
    uint64_t currentValue{};
    vkGetSemaphoreCounterValue(m_Device.logical, m_CPUSubmissionSemaphore,
                               &currentValue);
    return currentValue >= receipt.waitValue;
}

TextureFormat SYN::RenderDevice::getSwapchainFormat() const {
    TextureFormat format{};
    switch (m_Swapchain.format) {
    case VK_FORMAT_R8G8B8A8_SRGB:
        format = TextureFormat::rgba8;
        break;
    case VK_FORMAT_B8G8R8A8_SRGB:
        format = TextureFormat::bgra8;
        break;
    case VK_FORMAT_R8G8B8A8_UNORM:
        spdlog::info("Swapchain is not srgb");
        format = TextureFormat::rgba8;
        break;
    case VK_FORMAT_B8G8R8A8_UNORM:
        spdlog::info("Swapchain is not srgb");
        format = TextureFormat::bgra8;
        break;
    default:
        spdlog::warn("Swapchain is an unknown format, something "
                     "went wrong");
        assert(false);
        format = TextureFormat::bgra8;
    }

    return format;
}
TextureType SYN::RenderDevice::getSwapchainType() const {
    TextureType type{};
    switch (m_Swapchain.format) {
    case VK_FORMAT_R8G8B8A8_SRGB:
        type = TextureType::srgb;
        break;
    case VK_FORMAT_B8G8R8A8_SRGB:
        type = TextureType::srgb;
        break;
    case VK_FORMAT_R8G8B8A8_UNORM:
        spdlog::info("Swapchain is not srgb");
        type = TextureType::rgba;
        break;
    case VK_FORMAT_B8G8R8A8_UNORM:
        spdlog::info("Swapchain is not srgb");
        type = TextureType::rgba;
        break;
    default:
        spdlog::warn("Swapchain is an unknown format, something "
                     "went wrong");
        assert(false);
        type = TextureType::srgb;
    }

    return type;
}

namespace {
VkImageAspectFlags getVkAspect(TextureType type) {
    VkImageAspectFlags aspect{};
    switch (type) {
    case TextureType::srgb:
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    case TextureType::depth:
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
    case TextureType::rgba:
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    default:
        spdlog::warn("invalid texture type");
        assert(false);
        break;
    }

    return aspect;
}

VkFormat getVkFormat(TextureFormat format, TextureType type) {
    if (type == TextureType::invalid || format == TextureFormat::invalid) {
        spdlog::warn("invalid texture type or format");
        assert(false);
        return VK_FORMAT_R8G8B8A8_SRGB;
    }

    VkFormat vkFormat{};
    if (type == TextureType::depth) {
        if (format != TextureFormat::r32f) {
            spdlog::warn("Using wrong format for depth texture type");
            assert(false);
        }
        return VK_FORMAT_D32_SFLOAT;
    }

    if (type == TextureType::srgb) {
        if (format == TextureFormat::bgra8) {
            return VK_FORMAT_B8G8R8A8_SRGB;
        }
        if (format == TextureFormat::rgba8) {
            return VK_FORMAT_R8G8B8A8_SRGB;
        }
        spdlog::warn("Using wrong format for srgb texture type");
        assert(false);
    }

    switch (format) {
    case TextureFormat::rgba8:
        vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        break;
    case TextureFormat::rgba16f:
        vkFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        break;
    case TextureFormat::rgba32f:
        vkFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        break;
    case TextureFormat::r32f:
        vkFormat = VK_FORMAT_R32_SFLOAT;
        break;
    case TextureFormat::r32ui:
        vkFormat = VK_FORMAT_R32_UINT;
        break;
    case TextureFormat::bgra8:
        vkFormat = VK_FORMAT_B8G8R8A8_UNORM;
        break;
    default:
        spdlog::warn("invalid texture format");
        assert(false);
        break;
    }

    return vkFormat;
}
} // namespace
