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
    // very bad, use a deletion queue or something else later on
    vkDeviceWaitIdle(m_Device.logical);
    Buffer buffer{m_Buffers[handle]};

    m_Buffers.remove(handle);

    handle.id = UINT32_MAX;
}

TextureHandle SYN::VK::VulkanBackend::createTexture(const TextureDesc &desc) {
    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};
    VkImageAspectFlags aspect{};
    VkImageUsageFlags usage{VK_IMAGE_USAGE_TRANSFER_DST_BIT};
    switch (desc.type) {
    case TextureType::srgb:
        if (desc.usageMask & TEXTURE_USAGE_ATTACHMENT_BIT) {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        format = VK_FORMAT_R8G8B8A8_SRGB;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    case TextureType::depth:
        if (desc.usageMask & TEXTURE_USAGE_ATTACHMENT_BIT) {
            usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }
        format = VK_FORMAT_D32_SFLOAT;
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
    case TextureType::rgba:
        if (desc.usageMask & TEXTURE_USAGE_ATTACHMENT_BIT) {
            usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }
        format = VK_FORMAT_R8G8B8A8_UNORM;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    default:
        spdlog::warn("Could not add texture, invalid format");
        break;
    }

    if (desc.usageMask & TEXTURE_USAGE_SAMPLED_BIT) {
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    size_t nCopiesToCreate{1};
    if (desc.usageMask & TEXTURE_USAGE_ATTACHMENT_BIT) {
        nCopiesToCreate = c_MaxFramesInFlight;
    }

    std::array<Image, c_MaxFramesInFlight> images{};
    for (size_t i{}; i < nCopiesToCreate; i++) {
        Image image{createImage(m_Device, m_Allocator, format,
                                {.width = desc.width, .height = desc.height},
                                usage, aspect)};

        if (usage & VK_IMAGE_USAGE_SAMPLED_BIT) {
            if (m_BindlessTextureIndexFreelist.empty()) {
                spdlog::warn(
                    "Could not add texture to bindless array, array is full");
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

            vkUpdateDescriptorSets(m_Device.logical, 1, &bindlessWrite, 0,
                                   nullptr);
            image.bindlessTextureIndex = descriptorElement;
        }

        images[i] = std::move(image);
    }

    TextureHandle handle{};
    if (desc.usageMask & TEXTURE_USAGE_ATTACHMENT_BIT) {
        nCopiesToCreate = c_MaxFramesInFlight;
        handle = m_Attachments.insert(std::move(images));
    } else {
        handle = m_Textures.insert(images[0]);
    }

    return handle;
}

void SYN::VK::VulkanBackend::uploadToTexture(TextureHandle handle,
                                             uint32_t width, uint32_t height,
                                             uint32_t stride, void *data) {
    Image &texture{m_Textures[handle]};

    // TODO: have the transition happen automatically for all image types
    transitionImage(
        m_TransientCmdPool, m_Device, texture, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

    m_StagingBuffer.uploadToImage(m_Device, data, width, height, stride,
                                  texture);
    transitionImage(
        m_TransientCmdPool, m_Device, texture,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_SHADER_READ_BIT);
}

void SYN::VK::VulkanBackend::destroyTexture(TextureHandle &handle) {
    // VERY BAD, temporary, will make this better later
    vkDeviceWaitIdle(m_Device.logical);
    Image texture{m_Textures[handle]};

    destroyImage(m_Device, m_Allocator, texture);
    handle.id = UINT32_MAX;

    m_BindlessTextureIndexFreelist.emplace_back(texture.bindlessTextureIndex);
}
