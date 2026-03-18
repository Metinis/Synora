#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

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
} // namespace SYN::VK
