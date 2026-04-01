#pragma once

#include <queue>
#include <spdlog/spdlog.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "Buffer.h"

namespace SYN::VK {
struct Device;
struct Image;

class StagingBuffer {
  public:
    StagingBuffer &create(const Device &device, VmaAllocator allocator,
                          size_t size);
    void destroy(const Device &device, VmaAllocator allocator);

    void uploadToBuffer(const Device &device, const void *data, size_t size,
                        const Buffer &dstBuffer);

    void uploadToImage(const Device &device, const void *data, int width,
                       int height, const Image &dstImage);

    void stallOnPendingUploads(const Device &device);

  private:
    struct PendingUpload {
        VkCommandBuffer cmdBuffer;
        VkFence uploadFinishedFence;
        size_t nextReadCounter;
    };

    void poll(const Device &device);

    VkFence acquireFence(const Device &device);
    PendingUpload createPendingUpload(VkCommandBuffer cmdBuffer,
                                      VkFence fence) const;
    // resources must not be in flight or signalled
    void freePendingUpload(const Device &device,
                           const PendingUpload &pendingUpload);

    // returns offset to staged data in m_Buffer mapped memory
    size_t stageData(const Device &device, const void *data, size_t size);
    size_t getAvailableStagingSize();
    size_t getContiguousStagingSize();

    size_t m_WriteCounter{};
    size_t m_ReadCounter{};

    VkCommandPool m_CmdPool;
    Buffer m_Buffer;

    std::vector<VkFence> m_FencePool;

    std::queue<PendingUpload> m_PendingUploads;
};

} // namespace SYN::VK
