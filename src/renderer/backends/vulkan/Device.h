#pragma once

#include <vulkan/vulkan.h>

namespace SYN {
struct Device {
    VkPhysicalDevice physical;
    VkDevice logical;
};

Device createDevice(VkInstance instance, VkSurfaceKHR surface);
void destroyDevice(Device *device);
} // namespace SYN
