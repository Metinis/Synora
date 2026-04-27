#pragma once

#include "Buffer.h"
#include "Device.h"
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {
struct DynamicUBO {
  public:
    static constexpr VkDeviceSize c_MaxSizePerUBO{1024};

    void create(const Device &device, VmaAllocator allocator, size_t size);
    void destroy(VmaAllocator allocator);

    void reset();
    size_t padToAlignment(size_t size, size_t minAlignment);

    // returns dynamic offset
    uint32_t write(const void *data, size_t size);
    // returns the offset into the UBO
    template <typename T> VkDescriptorBufferInfo write(const T &data) {
        return write(&data, sizeof(data));
    }

    VkBuffer getVkBuffer() const;

  private:
    Buffer m_Buffer;
    size_t m_WriteOffset{};
    VkDeviceSize m_Alignment;
};
} // namespace SYN::VK
