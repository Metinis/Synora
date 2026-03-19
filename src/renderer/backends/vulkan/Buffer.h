#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {
struct Device;

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

} // namespace SYN::VK
