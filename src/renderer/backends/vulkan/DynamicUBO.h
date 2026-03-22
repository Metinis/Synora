#pragma once

#include "Buffer.h"
#include "Device.h"
#include <cstring>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {
struct DynamicUBO {
  public:
    void create(const Device &device, VmaAllocator allocator, size_t size);
    void destroy(VmaAllocator allocator);

    void reset();
    size_t padToAlignment(size_t size, size_t minAlignment);

    VkDescriptorBufferInfo write(const void *data, size_t size);
    // returns the offset into the UBO
    template <typename T> VkDescriptorBufferInfo write(const T &data) {
        return write(&data, sizeof(data));
    }

  private:
    Buffer m_Buffer;
    size_t m_WriteOffset{};
    VkDeviceSize m_Alignment;
};
} // namespace SYN::VK
