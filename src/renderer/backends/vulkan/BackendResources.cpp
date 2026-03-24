#include "Backend.h"
#include "core/Buffer.h"
#include "core/Commands.h"
#include "core/Device.h"
#include "core/Image.h"
#include "core/Pipeline.h"
#include "core/StagingBuffer.h"
#include "renderer/RenderTypes.h"

#include <GLFW/glfw3.h>
#include <cstring>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <vk_mem_alloc.h>

#include <PuzzleEngine/core/Window.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

BufferHandle SYN::VK::VulkanBackend::createBuffer(const BufferDesc &desc) {
    Buffer buffer{
        VK::createBuffer(m_Device, m_Allocator, desc.size,
                         VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                             VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         0)};

    BufferHandle handle{m_Buffers.insert(buffer)};

    return handle;
}

void SYN::VK::VulkanBackend::uploadToBuffer(BufferHandle handle, size_t size,
                                            void *data) {
    const Buffer &buffer{m_Buffers[handle]};
    m_StagingBuffer.uploadToBuffer(m_Device, data, size, buffer);
}

void SYN::VK::VulkanBackend::destroyBuffer(BufferHandle &handle) {
    Buffer buffer{m_Buffers[handle]};

    // the frame where all references to this texture will have completed
    size_t lastFrameInFlightIndex{
        (m_CurrentFrameIndex + (c_MaxFramesInFlight - 1)) %
        c_MaxFramesInFlight};
    m_FrameData[lastFrameInFlightIndex].buffersToFree.emplace_back(buffer);

    m_Buffers.remove(handle);

    handle.id = UINT32_MAX;
}

TextureHandle SYN::VK::VulkanBackend::createTexture(const TextureDesc &desc) {
    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};
    VkImageAspectFlags aspect{};
    VkImageUsageFlags usage{VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                            VK_IMAGE_USAGE_SAMPLED_BIT};
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

    Image image{createImage(m_Device, m_Allocator, format,
                            {.width = desc.width, .height = desc.height}, usage,
                            aspect)};

    if (m_BindlessTextureIndexFreelist.empty()) {
        spdlog::warn("Could not create texture, bindless array, array is full");
        destroyImage(m_Device, m_Allocator, image);
        return {};
    }
    uint32_t descriptorElement{m_BindlessTextureIndexFreelist.back()};
    m_BindlessTextureIndexFreelist.pop_back();

    VkDescriptorImageInfo imageInfo{
        .sampler = m_DefaultSampler,
        .imageView = image.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet bindlessWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_BindlessDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = descriptorElement,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
    };

    vkUpdateDescriptorSets(m_Device.logical, 1, &bindlessWrite, 0, nullptr);

    Texture texture{.image = std::move(image),
                    .bindlessSamplerIndex = descriptorElement};

    TextureHandle handle{m_Textures.insert(std::move(texture))};
    return handle;
}

void SYN::VK::VulkanBackend::uploadToTexture(TextureHandle handle,
                                             uint32_t width, uint32_t height,
                                             uint32_t stride, void *data) {
    Texture &texture{m_Textures[handle]};

    // TODO: have the transition happen automatically for all image types
    transitionImage(
        m_TransientCmdPool, m_Device, texture.image, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

    m_StagingBuffer.uploadToImage(m_Device, data, width, height, stride,
                                  texture.image);
    transitionImage(
        m_TransientCmdPool, m_Device, texture.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_SHADER_READ_BIT);
}

void SYN::VK::VulkanBackend::destroyTexture(TextureHandle &handle) {
    Texture texture{m_Textures[handle]};

    // the frame where all references to this texture will have completed
    size_t lastFrameInFlightIndex{
        (m_CurrentFrameIndex + (c_MaxFramesInFlight - 1)) %
        c_MaxFramesInFlight};

    m_FrameData[lastFrameInFlightIndex].imagesToFree.emplace_back(
        texture.image);
    m_FrameData[lastFrameInFlightIndex].bindlessTexturesToFree.emplace_back(
        texture.bindlessSamplerIndex);

    m_Textures.remove(handle);
    handle.id = UINT32_MAX;
}

AttachmentHandle
SYN::VK::VulkanBackend::createAttachment(const AttachmentDesc &desc) {
    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};
    VkImageAspectFlags aspect{};
    VkImageUsageFlags usage{};
    switch (desc.type) {
    case TextureType::srgb:
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        format = VK_FORMAT_R8G8B8A8_SRGB;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    case TextureType::depth:
        usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        format = VK_FORMAT_D32_SFLOAT;
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
    case TextureType::rgba:
        usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        format = VK_FORMAT_R8G8B8A8_UNORM;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    default:
        spdlog::warn("Could not add texture, invalid format");
        return {};
        break;
    }

    if (desc.isSampleable) {
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    uint32_t width{desc.width};
    uint32_t height{desc.height};
    if (desc.size == AttachmentSize::relative) {
        width = m_Swapchain.extent.width;
        height = m_Swapchain.extent.height;
    }

    std::array<Image, c_MaxFramesInFlight> images{};
    std::array<uint32_t, c_MaxFramesInFlight> bindlessSamplerIndices{};
    for (size_t i{}; i < images.size(); i++) {
        images[i] =
            createImage(m_Device, m_Allocator, format,
                        {.width = width, .height = height}, usage, aspect);

        if (!desc.isSampleable) {
            continue;
        }
        if (m_BindlessTextureIndexFreelist.empty()) {
            spdlog::warn("Could not create attachment, bindless array is full");

            destroyImage(m_Device, m_Allocator, images[i]);
            for (size_t j{}; j < i; j++) {
                destroyImage(m_Device, m_Allocator, images[j]);
                m_BindlessTextureIndexFreelist.emplace_back(
                    bindlessSamplerIndices[j]);
            }

            return {};
        }
        uint32_t descriptorElement{m_BindlessTextureIndexFreelist.back()};
        m_BindlessTextureIndexFreelist.pop_back();

        VkDescriptorImageInfo imageInfo{
            .sampler = m_DefaultSampler,
            .imageView = images[i].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };

        VkWriteDescriptorSet bindlessWrite{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_BindlessDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = descriptorElement,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo,
        };

        vkUpdateDescriptorSets(m_Device.logical, 1, &bindlessWrite, 0, nullptr);
        bindlessSamplerIndices[i] = descriptorElement;
    }

    Attachment attachment{
        .images = std::move(images),
        .bindlessSamplerIndices = std::move(bindlessSamplerIndices),
        .size = desc.size,
        .isSampleable = desc.isSampleable,
    };

    AttachmentHandle handle{m_Attachments.insert(std::move(attachment))};
    return handle;
}
void SYN::VK::VulkanBackend::destroyAttachment(AttachmentHandle &handle) {
    Attachment &attachment{m_Attachments[handle]};

    // the frame where all references to this texture will have completed
    size_t lastFrameInFlightIndex{
        (m_CurrentFrameIndex + (c_MaxFramesInFlight - 1)) %
        c_MaxFramesInFlight};
    for (auto &image : attachment.images) {
        m_FrameData[lastFrameInFlightIndex].imagesToFree.emplace_back(image);
    }

    if (attachment.isSampleable) {
        for (auto &index : attachment.bindlessSamplerIndices) {
            m_FrameData[lastFrameInFlightIndex]
                .bindlessTexturesToFree.emplace_back(index);
        }
    }

    m_Attachments.remove(handle);
    handle.id = UINT32_MAX;
}
