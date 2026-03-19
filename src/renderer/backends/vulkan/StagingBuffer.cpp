#include "StagingBuffer.h"
#include "Commands.h"
#include "Device.h"
#include "Image.h"

#include <cstdint>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

namespace {
void transitionImageCmd(const Device &device, const Image &image,
                        VkImageLayout oldLayout, VkImageLayout newLayout,
                        VkPipelineStageFlags2 srcStageMask,
                        VkAccessFlags2 srcAccessMask,
                        VkPipelineStageFlags2 dstStageMask,
                        VkAccessFlags2 dstAccessMask);
}

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

// TODO: make this batch writes, and if it does, probably make it a class
void SYN::VK::writeToBuffer(const Device &device, void *data, size_t size,
                            const StagingBuffer &stagingBuffer,
                            const Buffer &dstBuffer) {
    if (dstBuffer.size < size) {
        spdlog::warn("Could not write to buffer, write size larger than dst "
                     "buffer size");
        return;
    }

    auto chunkCount{static_cast<size_t>(
        std::ceil(static_cast<float>(size) /
                  static_cast<float>(stagingBuffer.buffer.size)))};

    for (size_t i{}; i < chunkCount; i++) {
        size_t dstOffset{(stagingBuffer.buffer.size * i)};
        size_t chunkSize{std::min(size - dstOffset, stagingBuffer.buffer.size)};

        memcpy(stagingBuffer.buffer.mappedData,
               static_cast<char *>(data) + dstOffset, chunkSize);

        submitImmediateCmd(stagingBuffer.cmdPool,
                           device.queues.at(QueueFamily::transfer).handle,
                           device, [&](VkCommandBuffer cmdBuffer) {
                               VkBufferCopy region{
                                   .dstOffset = dstOffset,
                                   .size = chunkSize,
                               };

                               vkCmdCopyBuffer(cmdBuffer,
                                               stagingBuffer.buffer.handle,
                                               dstBuffer.handle, 1, &region);
                           });
    }
}

void SYN::VK::writeToImageCmd(const Device &device, VkCommandBuffer cmdBuffer,
                              void *data, int width, int height,
                              size_t bytesPerPixel,
                              const StagingBuffer &stagingBuffer,
                              const Image &dstImage) {
    size_t rowSize{bytesPerPixel * dstImage.extent.width};
    size_t rowsPerChunk{stagingBuffer.buffer.size / rowSize};

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

    auto chunkCount{static_cast<size_t>(
        std::ceil(static_cast<float>(dstImage.extent.height) /
                  static_cast<float>(rowsPerChunk)))};

    for (size_t row{}; row < dstImage.extent.height; row += rowsPerChunk) {
        size_t rowsThisChunk{
            std::min(rowsPerChunk, dstImage.extent.height - row)};
        size_t chunkSize{rowsThisChunk * rowSize};

        memcpy(stagingBuffer.buffer.mappedData,
               static_cast<char *>(data) + (row * rowSize), chunkSize);

        VkBufferImageCopy region{
            .imageSubresource =
                {
                    .aspectMask = dstImage.subresourceRange.aspectMask,
                    .mipLevel = 0,
                    .baseArrayLayer = dstImage.subresourceRange.baseArrayLayer,
                    .layerCount = dstImage.subresourceRange.layerCount,
                },
            .imageOffset =
                {
                    .y = static_cast<int32_t>(row),
                },
            .imageExtent =
                {
                    .width = dstImage.extent.width,
                    .height = static_cast<uint32_t>(rowsThisChunk),
                    .depth = 1,
                },
        };

        vkCmdCopyBufferToImage(
            cmdBuffer, stagingBuffer.buffer.handle, dstImage.handle,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }
}

void SYN::VK::destroyStagingBuffer(StagingBuffer &stagingBuffer,
                                   const Device &device,
                                   VmaAllocator allocator) {
    vkDestroyCommandPool(device.logical, stagingBuffer.cmdPool, nullptr);
    destroyBuffer(allocator, stagingBuffer.buffer);
    stagingBuffer = {};
}

namespace {} // namespace
