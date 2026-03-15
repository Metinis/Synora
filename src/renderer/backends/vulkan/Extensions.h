#pragma once

#include <span>
#include <vulkan/vulkan.h>

namespace SYN::VK {
std::vector<const char *>
findExtensions(std::span<const char *> requiredExtensions,
               std::span<VkExtensionProperties> availableExtensionProperties);
}
