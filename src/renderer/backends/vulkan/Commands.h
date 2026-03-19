#pragma once

#include <functional>
#include <span>
#include <vulkan/vulkan.h>

namespace SYN::VK {
struct Device;

void submitImmediateCmd(VkCommandPool cmdPool, VkQueue queue,
                        const Device &device,
                        std::function<void(VkCommandBuffer)> immediateCmds);

VkCommandBuffer beginTransientCmd(VkCommandPool cmdPool, const Device &device);
void endTransientCmd(VkCommandPool cmdPool, VkCommandBuffer cmdBuffer,
                     const Device &device, VkQueue queue,
                     VkFence fence = VK_NULL_HANDLE,
                     std::span<VkSemaphore> waitSemaphores = {},
                     std::span<VkSemaphore> signalSemaphores = {});

} // namespace SYN::VK
