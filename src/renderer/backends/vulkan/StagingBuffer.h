#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "Buffer.h"

namespace SYN::VK {
struct Device;
struct Image;

struct StagingBuffer {
    VkCommandPool cmdPool;
    Buffer buffer;
};

StagingBuffer createStagingBuffer(const Device &device, VmaAllocator allocator,
                                  size_t size);

void writeToBuffer(const Device &device, void *data, size_t size,
                   const StagingBuffer &stagingBuffer, const Buffer &dstBuffer);

// src image must be same width and height as dst image
void writeToImageCmd(const Device &device, VkCommandBuffer cmdBuffer,
                     void *data, int width, int height, size_t bytesPerPixel,
                     const StagingBuffer &stagingBuffer, const Image &dstImage);

void destroyStagingBuffer(StagingBuffer &stagingBuffer, const Device &device,
                          VmaAllocator allocator);

} // namespace SYN::VK
