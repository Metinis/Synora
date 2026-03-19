#include "Buffer.h"
#include "Device.h"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

Buffer SYN::VK::createBuffer(const Device &device, VmaAllocator allocator,
                             size_t size, VkBufferUsageFlags usage,
                             VmaAllocationCreateFlags allocFlags) {
    VkBufferCreateInfo bufferCI = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    VmaAllocationCreateInfo allocInfo = {
        .flags = allocFlags,
        .usage = VMA_MEMORY_USAGE_AUTO,

    };

    VkBuffer buffer{};
    VmaAllocation allocation{};
    VmaAllocationInfo allocationInfo{};
    VkResult res{vmaCreateBuffer(allocator, &bufferCI, &allocInfo, &buffer,
                                 &allocation, &allocationInfo)};
    if (res != VK_SUCCESS) {
        spdlog::warn("Could not create Vulkan buffer. VkResult = {}",
                     static_cast<int>(res));
    }

    VkDeviceAddress deviceAddress{};
    if (usage & VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT) {
        VkBufferDeviceAddressInfo deviceAddressInfo{
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buffer};
        deviceAddress =
            vkGetBufferDeviceAddress(device.logical, &deviceAddressInfo);
    }

    return Buffer{.handle = buffer,
                  .allocation = allocation,
                  .size = size,
                  .mappedData = allocationInfo.pMappedData,
                  .deviceAddress = deviceAddress};
}

void SYN::VK::destroyBuffer(VmaAllocator allocator, Buffer &buffer) {
    vmaDestroyBuffer(allocator, buffer.handle, buffer.allocation);
    buffer = {};
}
