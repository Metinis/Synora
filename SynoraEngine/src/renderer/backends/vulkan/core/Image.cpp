#include "Image.h"
#include "Buffer.h"
#include "Commands.h"
#include "Device.h"
#include "Swapchain.h"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

namespace {} // namespace

Image SYN::VK::createImage(const Device &device, VmaAllocator allocator,
                           VkFormat format, VkExtent2D extent,
                           VkImageUsageFlags usage, VkImageAspectFlags aspect,
                           VkSampleCountFlagBits samples, uint32_t mipLevels,
                           uint32_t layerCount, bool isCubeMap) {
    VkImageViewType viewType{VK_IMAGE_VIEW_TYPE_2D};
    VkImageCreateFlags imageFlags{};
    if (isCubeMap) {
        if (layerCount % 6 != 0) {
            spdlog::error(
                "Could not create image, trying to create cubemap "
                "with layer count not a multiple of 6. Layer count = {}",
                layerCount);
            assert(false);
        }
        if (layerCount > 6) {
            spdlog::error("Could not create image, trying to create cubemap "
                          "with layer count greater than 6, cubemap array not "
                          "supported. Layer count = {}",
                          layerCount);
            assert(false);
        }

        imageFlags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    } else {
        if (layerCount > 1) {
            spdlog::error("Could not create image, trying to create image "
                          "with layer count greater than 1 thats not a "
                          "cubemap, texture array not "
                          "supported. Layer count = {}",
                          layerCount);
            assert(false);
        }
    }

    VkImageCreateInfo imageCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = imageFlags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {.width = extent.width, .height = extent.height, .depth = 1},
        .mipLevels = mipLevels,
        .arrayLayers = layerCount,
        .samples = samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocCI = {
        .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
    };

    VkImage handle{};
    VmaAllocation allocation{};
    VkResult res{vmaCreateImage(allocator, &imageCI, &allocCI, &handle,
                                &allocation, nullptr)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan image. VkResult = {}",
                      static_cast<int>(res));
    }

    VkImageViewCreateInfo regularViewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = handle,
        .viewType = viewType,
        .format = format,
        .subresourceRange{
            .aspectMask = aspect,
            .levelCount = mipLevels,
            .layerCount = layerCount,
        },
    };
    VkImageView regularView{};
    res = vkCreateImageView(device.logical, &regularViewCI, nullptr,
                            &regularView);
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan image view. VkResult = {}",
                      static_cast<int>(res));
    }

    std::vector<VkImageView> mipViews(mipLevels);
    for (uint32_t i{}; i < mipViews.size(); i++) {
        VkImageViewCreateInfo mipViewCI{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = handle,
            .viewType = viewType,
            .format = format,
            .subresourceRange{
                .aspectMask = aspect,
                .baseMipLevel = i,
                .levelCount = 1,
                .layerCount = layerCount,
            },
        };

        res = vkCreateImageView(device.logical, &mipViewCI, nullptr,
                                &mipViews[i]);
        if (res != VK_SUCCESS) {
            spdlog::error("Could not create Vulkan image view. VkResult = {}",
                          static_cast<int>(res));
        }
    }

    return Image{.handle = handle,
                 .view = regularView,
                 .mipViews = std::move(mipViews),
                 .extent = extent,
                 .subresourceRange = regularViewCI.subresourceRange,
                 .format = format,
                 .usage = usage,
                 .samples = samples,
                 .isCubeMap = isCubeMap,
                 .mipLevels = mipLevels,
                 .layerCount = layerCount,
                 .allocation = allocation};
}

void SYN::VK::destroyImage(const Device &device, VmaAllocator allocator,
                           Image &image) {
    vkDestroyImageView(device.logical, image.view, nullptr);
    for (auto view : image.mipViews) {
        vkDestroyImageView(device.logical, view, nullptr);
    }
    vmaDestroyImage(allocator, image.handle, image.allocation);

    image = {};
}

void SYN::VK::transitionImage(VkCommandPool cmdPool, const Device &device,
                              Image &image, VkImageLayout newLayout,
                              VkPipelineStageFlags2 dstStageMask,
                              VkAccessFlags2 dstAccessMask) {
    if (image.syncState.currentLayout == newLayout) {
        return;
    }

    VkCommandBuffer cmdBuffer{beginTransientCmd(cmdPool, device)};
    transitionImageCmd(cmdBuffer, image, newLayout, dstStageMask,
                       dstAccessMask);

    endTransientCmd(cmdBuffer);
    submitCmdBuffer(cmdPool, cmdBuffer, device,
                    device.queues.at(QueueFamily::transfer).handle);
}

void SYN::VK::transitionImageCmd(VkCommandBuffer cmdBuffer, Image &image,
                                 VkImageLayout targetLayout,
                                 VkPipelineStageFlags2 dstStageMask,
                                 VkAccessFlags2 dstAccessMask,
                                 uint32_t srcQueueFamilyIndex,
                                 uint32_t dstQueueFamilyIndex) {

    if (image.syncState.currentLayout == targetLayout) {
        return;
    }
    transitionImageCmd(cmdBuffer, image.handle, image.syncState.currentLayout,
                       targetLayout, image.syncState.lastStageMask,
                       image.syncState.lastAccessMask, dstStageMask,
                       dstAccessMask, image.subresourceRange,
                       srcQueueFamilyIndex, dstQueueFamilyIndex);

    image.syncState.currentLayout = targetLayout;
    image.syncState.lastAccessMask = dstAccessMask;
    image.syncState.lastStageMask = dstStageMask;
}

void SYN::VK::generateMipChain(VkCommandPool cmdPool, const Device &device,
                               Image &image, VkImageLayout newLayout,
                               VkPipelineStageFlags2 dstStageMask,
                               VkAccessFlags2 dstAccessMask) {
    if (image.mipLevels <= 1) {
        return;
    }
    VkFormatProperties formatProperties{};
    vkGetPhysicalDeviceFormatProperties(device.physical, image.format,
                                        &formatProperties);
    if (!(formatProperties.optimalTilingFeatures &
          VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        spdlog::warn("Could not create mip chain, linear filtering not "
                     "available on device");
        return;
    }

    VkCommandBuffer cmdBuffer{beginTransientCmd(cmdPool, device)};

    transitionImageCmd(
        cmdBuffer, image.handle, image.syncState.currentLayout,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image.syncState.lastStageMask,
        image.syncState.lastAccessMask, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VkImageSubresourceRange{
            .aspectMask = image.subresourceRange.aspectMask,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = image.subresourceRange.baseArrayLayer,
            .layerCount = image.subresourceRange.layerCount,
        });

    int32_t srcMipWidth{static_cast<int32_t>(image.extent.width)};
    int32_t srcMipHeight{static_cast<int32_t>(image.extent.height)};
    for (uint32_t i{1}; i < image.mipLevels; i++) {
        int32_t dstMipWidth{srcMipWidth};
        int32_t dstMipHeight{srcMipHeight};
        if (srcMipWidth > 1) {
            dstMipWidth /= 2;
        }
        if (srcMipHeight > 1) {
            dstMipHeight /= 2;
        }

        VkImageBlit blitRegion{
            .srcSubresource =
                {
                    .aspectMask = image.subresourceRange.aspectMask,
                    .mipLevel = i - 1,
                    .baseArrayLayer = image.subresourceRange.baseArrayLayer,
                    .layerCount = image.subresourceRange.layerCount,
                },

            .srcOffsets =
                {
                    {.x = 0, .y = 0, .z = 0},
                    {.x = srcMipWidth, .y = srcMipHeight, .z = 1},
                },
            .dstSubresource =
                {
                    .aspectMask = image.subresourceRange.aspectMask,
                    .mipLevel = i,
                    .baseArrayLayer = image.subresourceRange.baseArrayLayer,
                    .layerCount = image.subresourceRange.layerCount,
                },
            .dstOffsets =
                {
                    {.x = 0, .y = 0, .z = 0},
                    {.x = dstMipWidth, .y = dstMipHeight, .z = 1},
                },

        };

        transitionImageCmd(
            cmdBuffer, image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
            VkImageSubresourceRange{
                .aspectMask = image.subresourceRange.aspectMask,
                .baseMipLevel = i - 1,
                .levelCount = 1,
                .baseArrayLayer = image.subresourceRange.baseArrayLayer,
                .layerCount = image.subresourceRange.layerCount,
            });

        transitionImageCmd(
            cmdBuffer, image.handle, image.syncState.currentLayout,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image.syncState.lastStageMask,
            image.syncState.lastAccessMask,
            VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VkImageSubresourceRange{
                .aspectMask = image.subresourceRange.aspectMask,
                .baseMipLevel = i,
                .levelCount = 1,
                .baseArrayLayer = image.subresourceRange.baseArrayLayer,
                .layerCount = image.subresourceRange.layerCount,
            });

        vkCmdBlitImage(cmdBuffer, image.handle,
                       VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image.handle,
                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion,
                       VK_FILTER_LINEAR);

        transitionImageCmd(
            cmdBuffer, image.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            newLayout, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT, dstStageMask, dstAccessMask,
            VkImageSubresourceRange{
                .aspectMask = image.subresourceRange.aspectMask,
                .baseMipLevel = i - 1,
                .levelCount = 1,
                .baseArrayLayer = image.subresourceRange.baseArrayLayer,
                .layerCount = image.subresourceRange.layerCount,
            });

        srcMipWidth = dstMipWidth;
        srcMipHeight = dstMipHeight;
    }

    // last mip level isnt transitioned in for loop
    transitionImageCmd(
        cmdBuffer, image.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        newLayout, VK_PIPELINE_STAGE_2_ALL_TRANSFER_BIT,
        VK_ACCESS_2_TRANSFER_WRITE_BIT, dstStageMask, dstAccessMask,
        VkImageSubresourceRange{
            .aspectMask = image.subresourceRange.aspectMask,
            .baseMipLevel = image.mipLevels - 1,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        });

    endTransientCmd(cmdBuffer);
    submitCmdBuffer(cmdPool, cmdBuffer, device,
                    device.queues.at(QueueFamily::transfer).handle);

    image.syncState.lastAccessMask = dstAccessMask;
    image.syncState.lastStageMask = dstStageMask;
    image.syncState.currentLayout = newLayout;
}

void SYN::VK::transitionImageCmd(
    VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout srcLayout,
    VkImageLayout dstLayout, VkPipelineStageFlags2 srcStageMask,
    VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2 dstAccessMask, VkImageSubresourceRange subresourceRange,
    uint32_t srcQueueFamilyIndex, uint32_t dstQueueFamilyIndex) {

    if (srcLayout == dstLayout) {
        return;
    }

    VkImageMemoryBarrier2 barrier{.sType =
                                      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                  .srcStageMask = srcStageMask,
                                  .srcAccessMask = srcAccessMask,
                                  .dstStageMask = dstStageMask,
                                  .dstAccessMask = dstAccessMask,
                                  .oldLayout = srcLayout,
                                  .newLayout = dstLayout,
                                  .srcQueueFamilyIndex = srcQueueFamilyIndex,
                                  .dstQueueFamilyIndex = dstQueueFamilyIndex,
                                  .image = image,
                                  .subresourceRange = subresourceRange};

    VkDependencyInfo dependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };

    vkCmdPipelineBarrier2(cmdBuffer, &dependency);
}

VkSampleCountFlagBits SYN::VK::getSamples(const Device &device,
                                          uint32_t idealSampleCount) {
    VkSampleCountFlags sampleCountFlags{
        device.properties.limits.sampledImageColorSampleCounts &
        device.properties.limits.sampledImageDepthSampleCounts};
    if (idealSampleCount >= 64 && (sampleCountFlags & VK_SAMPLE_COUNT_64_BIT)) {
        return VK_SAMPLE_COUNT_64_BIT;
    }
    if (idealSampleCount >= 32 && (sampleCountFlags & VK_SAMPLE_COUNT_32_BIT)) {
        return VK_SAMPLE_COUNT_32_BIT;
    }
    if (idealSampleCount >= 16 && (sampleCountFlags & VK_SAMPLE_COUNT_16_BIT)) {
        return VK_SAMPLE_COUNT_16_BIT;
    }
    if (idealSampleCount >= 8 && (sampleCountFlags & VK_SAMPLE_COUNT_8_BIT)) {
        return VK_SAMPLE_COUNT_8_BIT;
    }
    if (idealSampleCount >= 4 && (sampleCountFlags & VK_SAMPLE_COUNT_4_BIT)) {
        return VK_SAMPLE_COUNT_4_BIT;
    }
    if (idealSampleCount >= 2 && (sampleCountFlags & VK_SAMPLE_COUNT_2_BIT)) {
        return VK_SAMPLE_COUNT_2_BIT;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}
