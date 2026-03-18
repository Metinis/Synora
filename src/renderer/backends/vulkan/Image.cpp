#include "Image.h"
#include "Device.h"
#include "renderer/backends/vulkan/Buffer.h"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

Image SYN::VK::createImage(const Device &device, VmaAllocator allocator,
                           VkFormat format, VkExtent2D extent,
                           VkImageUsageFlags usage,
                           VkImageAspectFlagBits aspect) {
    VkImageCreateInfo imageCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent = {.width = extent.width, .height = extent.height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
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

    VkImageViewCreateInfo viewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = handle,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = format,
        .subresourceRange{
            .aspectMask = aspect,
            .levelCount = 1,
            .layerCount = 1,
        },
    };
    VkImageView view{};
    res = vkCreateImageView(device.logical, &viewCI, nullptr, &view);
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan image view. VkResult = {}",
                      static_cast<int>(res));
    }

    return Image{.handle = handle,
                 .view = view,
                 .extent = extent,
                 .subresourceRange = viewCI.subresourceRange,
                 .allocation = allocation};
}

void SYN::VK::destroyImage(const Device &device, VmaAllocator allocator,
                           Image &image) {
    vkDestroyImageView(device.logical, image.view, nullptr);
    vmaDestroyImage(allocator, image.handle, image.allocation);

    image = {};
}

void SYN::VK::transitionImageCmd(
    VkCommandBuffer cmdBuffer, const Device &device, const Image &image,
    VkImageLayout oldLayout, VkImageLayout newLayout,
    VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
    VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask) {

    VkImageMemoryBarrier2 barrier{.sType =
                                      VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                  .srcStageMask = srcStageMask,
                                  .srcAccessMask = srcAccessMask,
                                  .dstStageMask = dstStageMask,
                                  .dstAccessMask = dstAccessMask,
                                  .oldLayout = oldLayout,
                                  .newLayout = newLayout,
                                  .image = image.handle,
                                  .subresourceRange = image.subresourceRange};

    VkDependencyInfo colorAttachmentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier,
    };
    vkCmdPipelineBarrier2(cmdBuffer, &colorAttachmentDependency);
}
