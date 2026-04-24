#include "RenderDevice.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "renderer/backends/vulkan/GraphicsCommandBuffer.h"
#include "renderer/backends/vulkan/UploadCommandBuffer.h"

using namespace SYN;
using namespace SYN::VK;

namespace {
constexpr uint32_t c_MB{1024 * 1024};
} // namespace

GraphicsCommandBuffer SYN::RenderDevice::acquireGraphicsCmdBuffer() {
    return GraphicsCommandBuffer(this);
}

UploadCommandBuffer SYN::RenderDevice::acquireUploadCmdBuffer() {
    return UploadCommandBuffer(this);
}

bool SYN::RenderDevice::beginFrame(Window &window) {
    m_StagingBuffer.stallOnPendingUploads(m_Device);

    FrameData &frame{getCurrentFrame()};

    vkWaitForFences(m_Device.logical, 1, &frame.renderFinishedFence, VK_TRUE,
                    UINT64_MAX);

    VkResult res{
        vkAcquireNextImageKHR(m_Device.logical, m_Swapchain.handle, UINT64_MAX,
                              frame.imageAvailableSemaphore, VK_NULL_HANDLE,
                              &m_CurrentSwapchainImageIndex)};

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        spdlog::info("Recreating swapchain");
        recreateSwapchain(window);
        return false;
    }

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        spdlog::error("Could not acquire swap chain image. VkResult = {}",
                      static_cast<int>(res));
        return false;
    }

    VkDescriptorBufferInfo bufferInfo{
        .buffer = frame.UBOBuffer.getVkBuffer(),
        .range = DynamicUBO::c_MaxSizePerUBO,
    };

    VkWriteDescriptorSet writeDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_UBODescriptorSet,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .pBufferInfo = &bufferInfo,
    };

    vkUpdateDescriptorSets(m_Device.logical, 1, &writeDescriptorSet, 0,
                           nullptr);

    vkResetCommandPool(m_Device.logical, frame.graphicsCmdPool, 0);

    return true;
}

void SYN::RenderDevice::submitWork(GraphicsCommandBuffer &cmdBuffer) {
    FrameData &frame{getCurrentFrame()};

    VkPipelineStageFlags waitStage{
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo queueSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame.imageAvailableSemaphore,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &frame.graphicsCmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores =
            &m_RenderFinishedSemaphores[m_CurrentSwapchainImageIndex],
    };

    vkResetFences(m_Device.logical, 1, &frame.renderFinishedFence);
    vkQueueSubmit(m_Device.queues.at(QueueFamily::graphics).handle, 1,
                  &queueSubmitInfo, frame.renderFinishedFence);

    frame.UBOBuffer.reset();
    m_CurrentFrameIndex =
        (m_CurrentFrameIndex + 1) % Limits::c_MaxFramesInFlight;

    cmdBuffer.m_RenderDevice = nullptr;
}

void SYN::RenderDevice::submitWork(UploadCommandBuffer &cmdBuffer) {
    cmdBuffer.m_RenderDevice = nullptr;
}

void SYN::RenderDevice::present(Window &window) {
    VkSwapchainKHR swapchainHandle{m_Swapchain.handle};
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &m_RenderFinishedSemaphores[m_CurrentSwapchainImageIndex],
        .swapchainCount = 1,
        .pSwapchains = &swapchainHandle,
        .pImageIndices = &m_CurrentSwapchainImageIndex,
    };

    VkResult res{vkQueuePresentKHR(
        m_Device.queues.at(QueueFamily::present).handle, &presentInfo)};

    if ((res == VK_SUBOPTIMAL_KHR) || (res == VK_ERROR_OUT_OF_DATE_KHR)) {
        spdlog::info("Recreating swapchain");
        recreateSwapchain(window);
    } else if (res != VK_SUCCESS) {
        spdlog::error("Could not present image. VkResult = {}",
                      static_cast<int>(res));
    }
}

AttachmentHandle SYN::RenderDevice::acquireSwapchainAttachment() {
    return m_SwapchainAttachmentHandles.at(m_CurrentSwapchainImageIndex);
}
