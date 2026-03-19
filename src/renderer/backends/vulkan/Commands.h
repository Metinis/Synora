#pragma once

#include <functional>
#include <vulkan/vulkan.h>

namespace SYN::VK {
struct Device;

void submitImmediateCmd(VkCommandPool cmdPool, VkQueue queue,
                        const Device &device,
                        std::function<void(VkCommandBuffer)> immediateCmds);
} // namespace SYN::VK
