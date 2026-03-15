#pragma once

#include <vulkan/vulkan.h>

namespace SYN::VK {

const VkDebugUtilsMessengerCreateInfoEXT *getDebugMessengerCreateInfo();

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance);

void destroyDebugMessenger(VkInstance instance,
                           VkDebugUtilsMessengerEXT debugMessenger);
} // namespace SYN
