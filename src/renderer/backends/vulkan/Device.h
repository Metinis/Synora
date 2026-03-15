#pragma once

#include <mutex>
#include <semaphore>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {

struct Queue {
    VkQueue handle;
    uint32_t familyIndex{};
};

enum class QueueFamily : uint32_t {
    graphics = 0,
    compute = 1,
    present = 2,
    transfer = 3,
    count
};

struct Device {
    VkPhysicalDevice physical;
    VkDevice logical;

    std::unordered_map<QueueFamily, Queue> queues;
};

Device createDevice(VkInstance instance, VkSurfaceKHR surface);
void destroyDevice(Device *device);
} // namespace SYN
