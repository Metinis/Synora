#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {
struct Device;
struct Swapchain;

struct Image {
    VkImage handle;
    VkImageView view;
    VkExtent2D extent;
    VkImageSubresourceRange subresourceRange;
    VkFormat format;
    VkImageUsageFlags usage;

    VmaAllocation allocation;
    VkImageLayout currentLayout{VK_IMAGE_LAYOUT_UNDEFINED};
    VkPipelineStageFlags2 lastStageMask{VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
    VkAccessFlags2 lastAccessMask{VK_ACCESS_2_MEMORY_READ_BIT |
                                  VK_ACCESS_2_MEMORY_WRITE_BIT};
};

Image createImage(const Device &device, VmaAllocator allocator, VkFormat format,
                  VkExtent2D extent, VkImageUsageFlags usage,
                  VkImageAspectFlags aspect);
void destroyImage(const Device &device, VmaAllocator allocator, Image &image);

void transitionImage(VkCommandPool cmdPool, const Device &device,
                     const Image &image, VkImageLayout oldLayout,
                     VkImageLayout newLayout,
                     VkPipelineStageFlags2 srcStageMask,
                     VkAccessFlags2 srcAccessMask,
                     VkPipelineStageFlags2 dstStageMask,
                     VkAccessFlags2 dstAccessMask);

void transitionImageCmd(VkCommandBuffer cmdBuffer, Image &image,
                        VkImageLayout targetLayout,
                        VkPipelineStageFlags2 dstStageMask,
                        VkAccessFlags2 dstAccessMask,
                        uint32_t srcQueueFamilyIndex = 0,
                        uint32_t dstQueueFamilyIndex = 0);
} // namespace SYN::VK
