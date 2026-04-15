#include "DynamicUBO.h"
#include "spdlog/spdlog.h"
#include <vulkan/vulkan_core.h>

void SYN::VK::DynamicUBO::reset() { m_WriteOffset = 0; }

size_t SYN::VK::DynamicUBO::padToAlignment(size_t size, size_t minAlignment) {
    return (size + (minAlignment - 1)) & ~(minAlignment - 1);
}

uint32_t SYN::VK::DynamicUBO::write(const void *data, size_t size) {
    if (size > c_MaxSizePerUBO) {
        spdlog::warn("Trying to write a UBO of size {} which is larger than "
                     "the max of {}, clamping",
                     c_MaxSizePerUBO, size);
        size = c_MaxSizePerUBO;
    }
    size_t offset{padToAlignment(m_WriteOffset, m_Alignment)};
    if ((offset + size) > m_Buffer.size) {
        spdlog::warn("Could not write to UBOBuffer, buffer is full");
        assert(false);
        return {};
    }
    memcpy(static_cast<uint8_t *>(m_Buffer.mappedData) + offset, data, size);

    VkDescriptorBufferInfo info{
        .buffer = m_Buffer.handle, .offset = offset, .range = size};

    m_WriteOffset = offset + size;
    return offset;
}

void SYN::VK::DynamicUBO::create(const Device &device, VmaAllocator allocator,
                                 size_t size) {
    m_Buffer = createBuffer(
        device, allocator, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_ALLOCATION_CREATE_MAPPED_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    m_Alignment = device.properties.limits.minUniformBufferOffsetAlignment;
}
void SYN::VK::DynamicUBO::destroy(VmaAllocator allocator) {
    destroyBuffer(allocator, m_Buffer);
}

VkBuffer SYN::VK::DynamicUBO::getVkBuffer() const { return m_Buffer.handle; }
