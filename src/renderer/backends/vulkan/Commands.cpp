#include "Commands.h"
#include "Device.h"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

void SYN::VK::submitImmediateCmd(
    VkCommandPool cmdPool, VkQueue queue, const Device &device,
    std::function<void(VkCommandBuffer)> immediateCmds) {
    VkCommandBuffer cmdBuffer{beginTransientCmd(cmdPool, device)};

    immediateCmds(cmdBuffer);

    endTransientCmd(cmdPool, cmdBuffer, device, queue);
}

VkCommandBuffer SYN::VK::beginTransientCmd(VkCommandPool cmdPool,
                                           const Device &device) {
    VkCommandBufferAllocateInfo cmdBufferAllocInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = cmdPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmdBuffer{};
    VkResult res{vkAllocateCommandBuffers(device.logical, &cmdBufferAllocInfo,
                                          &cmdBuffer)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not allocate command buffers, VkResult = {}",
                      static_cast<int>(res));
    }

    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    res = vkBeginCommandBuffer(cmdBuffer, &cmdBufferBeginInfo);
    if (res != VK_SUCCESS) {
        spdlog::error("Could not begin command buffer, VkResult = {}",
                      static_cast<int>(res));
    }

    return cmdBuffer;
}
void SYN::VK::endTransientCmd(VkCommandPool cmdPool, VkCommandBuffer cmdBuffer,
                              const Device &device, VkQueue queue,
                              VkFence fence,
                              std::span<VkSemaphore> waitSemaphores,
                              std::span<VkSemaphore> signalSemaphores) {
    VkResult res{vkEndCommandBuffer(cmdBuffer)};

    if (res != VK_SUCCESS) {
        spdlog::error("Could not end command buffer, VkResult = {}",
                      static_cast<int>(res));
    }

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphores = waitSemaphores.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer,
        .signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size()),
        .pSignalSemaphores = signalSemaphores.data(),
    };

    if (fence == VK_NULL_HANDLE) {
        VkFenceCreateInfo fenceInfo{.sType =
                                        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        vkCreateFence(device.logical, &fenceInfo, nullptr, &fence);
    }

    res = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (res != VK_SUCCESS) {
        spdlog::error("Could not submit command buffer, VkResult = {}",
                      static_cast<int>(res));
    }

    vkWaitForFences(device.logical, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device.logical, fence, nullptr);
    vkFreeCommandBuffers(device.logical, cmdPool, 1, &cmdBuffer);
}
