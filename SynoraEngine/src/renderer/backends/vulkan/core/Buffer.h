#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {
struct Device;
struct Image;

struct Buffer {
    VkBuffer handle;
    VmaAllocation allocation;
    size_t size;
    void *mappedData;
    VkDeviceAddress deviceAddress;
};

Buffer createBuffer(const Device &device, VmaAllocator allocator, size_t size,
                    VkBufferUsageFlags usage,
                    VmaAllocationCreateFlags allocFlags);

void destroyBuffer(VmaAllocator allocator, Buffer &buffer);

void copyImageToBufferCmd(VkCommandBuffer cmdBuffer, Buffer &buffer,
                          const Image &image, uint32_t mipLevel);

} // namespace SYN::VK
