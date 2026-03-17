#include "StagingBuffer.h"
#include "Commands.h"
#include "Device.h"

#include <cstdint>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

StagingBuffer SYN::VK::createStagingBuffer(const Device &device,
                                           VmaAllocator allocator,
                                           size_t size) {

    Buffer buffer{
        createBuffer(device, allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT)};

    VkCommandPoolCreateInfo transientCmdPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex =
            device.queues.at(QueueFamily::transfer).familyIndex};

    VkCommandPool cmdPool{};
    VkResult res{vkCreateCommandPool(device.logical, &transientCmdPoolCI,
                                     nullptr, &cmdPool)};
    if (res != VK_SUCCESS) {
        spdlog::error(
            "Could not create staging buffer command pool. VkResult = {}",
            static_cast<int>(res));
    }

    return StagingBuffer{
        .cmdPool = cmdPool,
        .buffer = buffer,
    };
}

void SYN::VK::writeToBuffer(const Device &device, void *srcData, size_t size,
                            const StagingBuffer &stagingBuffer,
                            const Buffer &dstBuffer) {
    if (dstBuffer.size < size) {
        spdlog::warn("Could not write to buffer, write size larger than dst "
                     "buffer size");
        return;
    }

    auto stagingBufferSizeMultiples{static_cast<size_t>(
        std::ceil(static_cast<float>(size) /
                  static_cast<float>(stagingBuffer.buffer.size)))};

    for (size_t i{}; i < stagingBufferSizeMultiples; i++) {
        size_t dstOffset{(stagingBuffer.buffer.size * i)};
        auto newData{static_cast<char *>(srcData) + dstOffset};
        size_t srcSize{std::min(size - dstOffset, stagingBuffer.buffer.size)};

        memcpy(stagingBuffer.buffer.mappedData, newData, srcSize);

        submitImmediateCmd(stagingBuffer.cmdPool,
                           device.queues.at(QueueFamily::transfer).handle,
                           device, [&](VkCommandBuffer cmdBuffer) {
                               VkBufferCopy region{
                                   .dstOffset = dstOffset,
                                   .size = srcSize,
                               };

                               vkCmdCopyBuffer(cmdBuffer,
                                               stagingBuffer.buffer.handle,
                                               dstBuffer.handle, 1, &region);
                           });
    }
}

void SYN::VK::destroyStagingBuffer(StagingBuffer &stagingBuffer,
                                   const Device &device,
                                   VmaAllocator allocator) {
    vkDestroyCommandPool(device.logical, stagingBuffer.cmdPool, nullptr);
    destroyBuffer(allocator, stagingBuffer.buffer);
    stagingBuffer = {};
}
