#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {
struct Device;
struct Swapchain;

struct ImageSyncState {
    VkImageLayout currentLayout{VK_IMAGE_LAYOUT_UNDEFINED};
    VkPipelineStageFlags2 lastStageMask{VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT};
    VkAccessFlags2 lastAccessMask{VK_ACCESS_2_MEMORY_READ_BIT |
                                  VK_ACCESS_2_MEMORY_WRITE_BIT};
};

struct Image {
    VkImage handle;
    VkImageView view;
    VkExtent2D extent;
    VkImageSubresourceRange subresourceRange;
    VkFormat format;
    VkImageUsageFlags usage;

    uint32_t mipLevels{1};

    VmaAllocation allocation;
    ImageSyncState syncState;
};

// creates with optimal tiling
Image createImage(const Device &device, VmaAllocator allocator, VkFormat format,
                  VkExtent2D extent, VkImageUsageFlags usage,
                  VkImageAspectFlags aspect, uint32_t mipLevels = 1);
void destroyImage(const Device &device, VmaAllocator allocator, Image &image);

// does nothing if target layout matches current layout
void transitionImage(VkCommandPool cmdPool, const Device &device, Image &image,
                     VkImageLayout newLayout,
                     VkPipelineStageFlags2 dstStageMask,
                     VkAccessFlags2 dstAccessMask);

// does nothing if target layout matches current layout
void transitionImageCmd(VkCommandBuffer cmdBuffer, Image &image,
                        VkImageLayout targetLayout,
                        VkPipelineStageFlags2 dstStageMask,
                        VkAccessFlags2 dstAccessMask,
                        uint32_t srcQueueFamilyIndex = 0,
                        uint32_t dstQueueFamilyIndex = 0);

void transitionImageCmd(
    VkCommandBuffer cmdBuffer, VkImage image, VkImageLayout srcLayout,
    VkImageLayout dstLayout, VkPipelineStageFlags2 srcStageMask,
    VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask,
    VkAccessFlags2 dstAccessMask, VkImageSubresourceRange subresourceRange,
    uint32_t srcQueueFamilyIndex = 0, uint32_t dstQueueFamilyIndex = 0);

// postcondition: all image subresources will be transitioned to the new
// layout. precondition: image should have mip level 0 uploaded
void generateMipChain(VkCommandPool cmdPool, const Device &device, Image &image,
                      VkImageLayout newLayout,
                      VkPipelineStageFlags2 dstStageMask,
                      VkAccessFlags2 dstAccessMask);
} // namespace SYN::VK
