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
void endTransientCmd(VkCommandBuffer cmdBuffer);

// frees cmdBuffer when fence is not provided
void submitCmdBuffers(VkCommandPool cmdPool,
                      std::span<VkCommandBuffer> cmdBuffers,
                      const Device &device, VkQueue queue,
                      VkFence fence = VK_NULL_HANDLE,
                      std::span<VkSemaphore> waitSemaphores = {},
                      std::span<VkSemaphore> signalSemaphores = {});

// frees cmdBuffer when fence is not provided
void submitCmdBuffer(VkCommandPool cmdPool, VkCommandBuffer cmdBuffer,
                     const Device &device, VkQueue queue,
                     VkFence fence = VK_NULL_HANDLE,
                     std::span<VkSemaphore> waitSemaphores = {},
                     std::span<VkSemaphore> signalSemaphores = {});

} // namespace SYN::VK
