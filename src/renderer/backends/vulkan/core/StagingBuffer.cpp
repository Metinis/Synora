#include "StagingBuffer.h"
#include "Commands.h"
#include "Device.h"
#include "Image.h"

#include <cstdint>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

StagingBuffer &SYN::VK::StagingBuffer::create(const Device &device,
                                              VmaAllocator allocator,
                                              size_t size) {
    m_Buffer =
        createBuffer(device, allocator, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                     VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                         VMA_ALLOCATION_CREATE_MAPPED_BIT);

    VkCommandPoolCreateInfo transientCmdPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex =
            device.queues.at(QueueFamily::transfer).familyIndex};

    VkResult res{vkCreateCommandPool(device.logical, &transientCmdPoolCI,
                                     nullptr, &m_CmdPool)};
    if (res != VK_SUCCESS) {
        spdlog::error(
            "Could not create staging buffer command pool. VkResult = {}",
            static_cast<int>(res));
    }

    return *this;
}

void SYN::VK::StagingBuffer::destroy(const Device &device,
                                     VmaAllocator allocator) {
    stallOnPendingUploads(device);
    for (auto fence : m_FencePool) {
        vkDestroyFence(device.logical, fence, nullptr);
    }

    vkDestroyCommandPool(device.logical, m_CmdPool, nullptr);
    destroyBuffer(allocator, m_Buffer);
}

size_t SYN::VK::StagingBuffer::getAvailableStagingSize() {
    return m_Buffer.size -
           (m_WriteCounter - m_ReadCounter); // --|(RC)----|(WC)----
}

size_t SYN::VK::StagingBuffer::getContiguousStagingSize() {
    // size_t writeCursor{m_WriteCounter % m_Buffer.size};
    // size_t readCursor{m_ReadCounter % m_Buffer.size};

    assert(m_ReadCounter <= m_WriteCounter);
    assert(m_Buffer.size >= (m_WriteCounter - m_ReadCounter));
    return m_Buffer.size - (m_WriteCounter - m_ReadCounter);

    // if (writeCursor > readCursor) {
    //     return m_Buffer.size - writeCursor;
    // } else if (writeCursor == readCursor) {
    //     if (m_WriteCounter == m_ReadCounter) {
    //         return m_Buffer.size;
    //     }
    //     return 0;
    // }
    // return readCursor - writeCursor;
}

void SYN::VK::StagingBuffer::poll(const Device &device) {
    while (!m_PendingUploads.empty()) {
        PendingUpload pending{m_PendingUploads.front()};
        if (vkGetFenceStatus(device.logical, pending.uploadFinishedFence) ==
            VK_SUCCESS) {
            m_ReadCounter = pending.nextReadCounter;
            freePendingUpload(device, pending);
            m_PendingUploads.pop();
        } else {
            break;
        }
    }
}

VkFence SYN::VK::StagingBuffer::acquireFence(const Device &device) {
    VkFence fence{};
    if (m_FencePool.empty()) {
        VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        VkResult res{vkCreateFence(device.logical, &fenceCI, nullptr, &fence)};
        if (res != VK_SUCCESS) {
            spdlog::error("Could not create Vulkan Fence. VkResult = {}",
                          static_cast<int>(res));
        }
    } else {
        fence = m_FencePool.back();
        m_FencePool.pop_back();
    }

    return fence;
}

StagingBuffer::PendingUpload
SYN::VK::StagingBuffer::createPendingUpload(VkCommandBuffer cmdBuffer,
                                            VkFence fence) const {
    return PendingUpload{.cmdBuffer = cmdBuffer,
                         .uploadFinishedFence = fence,
                         .nextReadCounter = m_WriteCounter};
}

void SYN::VK::StagingBuffer::freePendingUpload(
    const Device &device, const PendingUpload &pendingUpload) {

    vkResetFences(device.logical, 1, &pendingUpload.uploadFinishedFence);
    m_FencePool.emplace_back(pendingUpload.uploadFinishedFence);
    vkFreeCommandBuffers(device.logical, m_CmdPool, 1,
                         &pendingUpload.cmdBuffer);
}

void SYN::VK::StagingBuffer::stallOnPendingUploads(const Device &device) {
    poll(device);

    while (!m_PendingUploads.empty()) {
        PendingUpload pendingUpload{m_PendingUploads.front()};

        vkWaitForFences(device.logical, 1, &pendingUpload.uploadFinishedFence,
                        VK_TRUE, UINT64_MAX);
        m_ReadCounter = pendingUpload.nextReadCounter;
        freePendingUpload(device, pendingUpload);
        m_PendingUploads.pop();
    }
}

size_t SYN::VK::StagingBuffer::stageData(const Device &device, const void *data,
                                         size_t size) {
    if (size > m_Buffer.size) {
        spdlog::warn("Could not write all bytes to staging buffer, trying to "
                     "write more bytes than staging buffer size");
        size = m_Buffer.size;
    }

    poll(device);
    stallOnPendingUploads(device);
    while (size > getContiguousStagingSize()) {
        PendingUpload pendingUpload{m_PendingUploads.front()};

        vkWaitForFences(device.logical, 1, &pendingUpload.uploadFinishedFence,
                        VK_TRUE, UINT64_MAX);
        m_ReadCounter = pendingUpload.nextReadCounter;
        freePendingUpload(device, pendingUpload);
        m_PendingUploads.pop();
    }

    size_t writeCursor{m_WriteCounter - m_ReadCounter};
    assert((writeCursor + size) <= m_Buffer.size);

    memcpy(static_cast<uint8_t *>(m_Buffer.mappedData) + writeCursor, data,
           size);

    m_WriteCounter = m_WriteCounter + size;
    return writeCursor;
}

void SYN::VK::StagingBuffer::uploadToBuffer(const Device &device,
                                            const void *data, size_t size,
                                            const Buffer &dstBuffer) {
    if (dstBuffer.size < size) {
        spdlog::warn("Could not write to buffer, write size larger than dst "
                     "buffer size");
        return;
    }

    size_t bytesLeft{size};
    while (bytesLeft) {
        size_t dstOffset{size - bytesLeft};
        size_t chunkSize{std::min(bytesLeft, m_Buffer.size)};
        bytesLeft -= chunkSize;

        size_t stagedOffset{stageData(
            device, static_cast<const char *>(data) + dstOffset, chunkSize)};

        VkCommandBuffer cmdBuffer{beginTransientCmd(m_CmdPool, device)};
        {

            VkBufferCopy copyRegion{
                .srcOffset = stagedOffset,
                .dstOffset = dstOffset,
                .size = chunkSize,
            };

            vkCmdCopyBuffer(cmdBuffer, m_Buffer.handle, dstBuffer.handle, 1,
                            &copyRegion);
        }

        endTransientCmd(cmdBuffer);

        VkFence fence{acquireFence(device)};
        submitCmdBuffer(m_CmdPool, cmdBuffer, device,
                        device.queues.at(QueueFamily::transfer).handle, fence);
        m_PendingUploads.push(createPendingUpload(cmdBuffer, fence));
    }
}
void SYN::VK::StagingBuffer::uploadToImage(const Device &device,
                                           const void *data, int width,
                                           int height, const Image &dstImage) {
    constexpr uint32_t bytesPerPixel{4};
    uint32_t rowSize{bytesPerPixel * dstImage.extent.width};
    uint32_t rowsPerChunk{static_cast<uint32_t>(m_Buffer.size / rowSize)};

    if (rowsPerChunk == 0) {
        spdlog::warn("Could not write to Vulkan image, one image row did not "
                     "fit in staging buffer");
        assert(false);
        return;
    }

    if (width != dstImage.extent.width || height != dstImage.extent.height) {
        spdlog::warn("Could not write to Vulkan image, src image width or "
                     "height does not match dst image width or height");
        assert(false);
        return;
    }

    uint32_t rowsLeft{static_cast<uint32_t>(height)};

    if ((rowsLeft * rowSize) > m_Buffer.size) {
        spdlog::warn("Trying to upload image thats larger than staging buffer");
    }

    while (rowsLeft) {
        uint32_t currentRow{height - rowsLeft};
        uint32_t dstOffset{currentRow * rowSize};

        size_t rowsThisChunk{std::min(rowsPerChunk, rowsLeft)};
        rowsLeft -= rowsThisChunk;
        size_t chunkSize{rowsThisChunk * rowSize};

        size_t stagedOffset{stageData(
            device, static_cast<const uint8_t *>(data) + dstOffset, chunkSize)};

        VkCommandBuffer cmdBuffer{beginTransientCmd(m_CmdPool, device)};
        {

            VkBufferImageCopy copyRegion{
                .bufferOffset = stagedOffset,
                .imageSubresource =
                    {
                        .aspectMask = dstImage.subresourceRange.aspectMask,
                        .mipLevel = 0,
                        .baseArrayLayer =
                            dstImage.subresourceRange.baseArrayLayer,
                        .layerCount = dstImage.subresourceRange.layerCount,
                    },
                .imageOffset =
                    {
                        .y = static_cast<int32_t>(currentRow),
                    },
                .imageExtent =
                    {
                        .width = dstImage.extent.width,
                        .height = static_cast<uint32_t>(rowsThisChunk),
                        .depth = 1,
                    },
            };

            vkCmdCopyBufferToImage(cmdBuffer, m_Buffer.handle, dstImage.handle,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                   &copyRegion);
        }
        endTransientCmd(cmdBuffer);

        VkFence fence{acquireFence(device)};
        submitCmdBuffer(m_CmdPool, cmdBuffer, device,
                        device.queues.at(QueueFamily::transfer).handle, fence);
        m_PendingUploads.push(createPendingUpload(cmdBuffer, fence));
    }
}
