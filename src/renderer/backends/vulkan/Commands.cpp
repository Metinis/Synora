#include "Commands.h"
#include "Device.h"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

namespace {
VkCommandBuffer beginTransientCmd(VkCommandPool cmdPool, const Device &device);
void endTransientCmd(VkCommandPool cmdPool, VkCommandBuffer cmdBuffer,
                     const Device &device, VkQueue queue);
} // namespace

void SYN::VK::submitImmediateCmd(
    VkCommandPool cmdPool, VkQueue queue, const Device &device,
    std::function<void(VkCommandBuffer)> immediateCmds) {
    VkCommandBuffer cmdBuffer{beginTransientCmd(cmdPool, device)};

    immediateCmds(cmdBuffer);

    endTransientCmd(cmdPool, cmdBuffer, device, queue);
}

namespace {
VkCommandBuffer beginTransientCmd(VkCommandPool cmdPool, const Device &device) {
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
void endTransientCmd(VkCommandPool cmdPool, VkCommandBuffer cmdBuffer,
                     const Device &device, VkQueue queue) {
    VkResult res{vkEndCommandBuffer(cmdBuffer)};

    if (res != VK_SUCCESS) {
        spdlog::error("Could not end command buffer, VkResult = {}",
                      static_cast<int>(res));
    }

    VkSubmitInfo submitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &cmdBuffer,
    };

    VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    VkFence fence{};
    vkCreateFence(device.logical, &fenceInfo, nullptr, &fence);

    res = vkQueueSubmit(queue, 1, &submitInfo, fence);
    if (res != VK_SUCCESS) {
        spdlog::error("Could not submit command buffer, VkResult = {}",
                      static_cast<int>(res));
    }

    vkWaitForFences(device.logical, 1, &fence, VK_TRUE, UINT64_MAX);

    vkDestroyFence(device.logical, fence, nullptr);
    vkFreeCommandBuffers(device.logical, cmdPool, 1, &cmdBuffer);
}
} // namespace
