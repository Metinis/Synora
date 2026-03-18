#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {
struct Device;

struct Image {
    VkImage handle;
    VkImageView view;
    VkExtent2D extent;
    VkImageSubresourceRange subresourceRange;

    VmaAllocation allocation;
};

Image createImage(const Device &device, VmaAllocator allocator, VkFormat format,
                  VkExtent2D extent, VkImageUsageFlags usage,
                  VkImageAspectFlagBits aspect);
void destroyImage(const Device &device, VmaAllocator allocator, Image &image);

void transitionImageCmd(VkCommandBuffer cmdBuffer, const Device &device,
                        const Image &image, VkImageLayout oldLayout,
                        VkImageLayout newLayout,
                        VkPipelineStageFlags2 srcStageMask,
                        VkAccessFlags2 srcAccessMask,
                        VkPipelineStageFlags2 dstStageMask,
                        VkAccessFlags2 dstAccessMask);
} // namespace SYN::VK
