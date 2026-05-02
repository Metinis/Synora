#include "RenderDevice.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include <cstdint>
#include <vulkan/vulkan_core.h>

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

ComputeCommandBuffer SYN::RenderDevice::acquireComputeCmdBuffer() {
    return ComputeCommandBuffer(this);
}

bool SYN::RenderDevice::beginFrame(Window &window) {
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
        .dstSet = m_UBOSet,
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

SYN::Receipt SYN::RenderDevice::submitWork(GraphicsCommandBuffer &cmdBuffer,
                                           std::span<Receipt> waitReceipts) {
    FrameData &frame{getCurrentFrame()};
    m_SubmissionCount++;

    AttachmentHandle swapchainHandle{
        m_SwapchainAttachmentHandles.at(m_CurrentSwapchainImageIndex)};

    transitionImageCmd(frame.graphicsCmdBuffer,
                       m_Attachments[swapchainHandle].image,
                       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                       VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE);

    vkEndCommandBuffer(frame.graphicsCmdBuffer);

    std::vector<VkSemaphoreSubmitInfo> waitSemaphores{
        VkSemaphoreSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = frame.imageAvailableSemaphore,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        },
    };
    waitSemaphores.reserve(waitReceipts.size());
    for (const auto &receipt : waitReceipts) {
        VkSemaphoreSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_GPUSubmissionSemaphore,
            .value = receipt.waitValue,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
        };
        waitSemaphores.emplace_back(submitInfo);
    }
    std::vector<VkSemaphoreSubmitInfo> signalSemaphores{
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore =
                m_RenderFinishedSemaphores[m_CurrentSwapchainImageIndex],
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_GPUSubmissionSemaphore,
            .value = m_SubmissionCount,
            .stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_CPUSubmissionSemaphore,
            .value = m_SubmissionCount,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        },

    };

    VkCommandBufferSubmitInfo cmdBufSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = frame.graphicsCmdBuffer};

    VkSubmitInfo2 queueSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,

        .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphoreInfos = waitSemaphores.data(),
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdBufSubmitInfo,
        .signalSemaphoreInfoCount =
            static_cast<uint32_t>(signalSemaphores.size()),
        .pSignalSemaphoreInfos = signalSemaphores.data(),
    };

    vkResetFences(m_Device.logical, 1, &frame.renderFinishedFence);
    vkQueueSubmit2(m_Device.queues.at(QueueFamily::graphics).handle, 1,
                   &queueSubmitInfo, frame.renderFinishedFence);

    frame.UBOBuffer.reset();

    m_CurrentFrameIndex =
        (m_CurrentFrameIndex + 1) % Limits::c_MaxFramesInFlight;

    cmdBuffer.reset();

    return Receipt{.waitValue = m_SubmissionCount};
}

SYN::Receipt SYN::RenderDevice::submitWork(UploadCommandBuffer &cmdBuffer,
                                           std::span<Receipt> waitReceipts) {
    if (waitReceipts.size() > 0) {
        // TODO: make this use the gpu semaphore
        std::vector<VkSemaphore> waitSemaphores{};
        std::vector<uint64_t> waitValues{};
        waitSemaphores.reserve(waitReceipts.size());
        waitValues.reserve(waitReceipts.size());

        for (const auto &receipt : waitReceipts) {
            waitSemaphores.emplace_back(m_CPUSubmissionSemaphore);
            waitValues.emplace_back(receipt.waitValue);
        }
        VkSemaphoreWaitInfo waitInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphoreCount = static_cast<uint32_t>(waitReceipts.size()),
            .pSemaphores = waitSemaphores.data(),
            .pValues = waitValues.data(),
        };

        vkWaitSemaphores(m_Device.logical, &waitInfo, UINT64_MAX);
    }

    cmdBuffer.reset();
    return {
        .waitValue = 0 // zero cus we waited on the cpu so the data is available
                       // immediately
    };
}

SYN::Receipt SYN::RenderDevice::submitWork(ComputeCommandBuffer &cmdBuffer,
                                           std::span<Receipt> waitReceipts) {
    m_SubmissionCount++;

    vkEndCommandBuffer(cmdBuffer.m_CmdBuffer);

    std::vector<VkSemaphoreSubmitInfo> waitSemaphores{};
    waitSemaphores.reserve(waitReceipts.size());
    for (const auto &receipt : waitReceipts) {
        VkSemaphoreSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_GPUSubmissionSemaphore,
            .value = receipt.waitValue,
            .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        };
        waitSemaphores.emplace_back(submitInfo);
    }

    std::vector<VkSemaphoreSubmitInfo> signalSemaphores{
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_GPUSubmissionSemaphore,
            .value = m_SubmissionCount,
            .stageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        },
        {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = m_CPUSubmissionSemaphore,
            .value = m_SubmissionCount,
            .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        },

    };

    VkCommandBufferSubmitInfo cmdBufSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = cmdBuffer.m_CmdBuffer};

    VkSubmitInfo2 queueSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount = static_cast<uint32_t>(waitSemaphores.size()),
        .pWaitSemaphoreInfos = waitSemaphores.data(),
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &cmdBufSubmitInfo,
        .signalSemaphoreInfoCount =
            static_cast<uint32_t>(signalSemaphores.size()),
        .pSignalSemaphoreInfos = signalSemaphores.data(),

    };

    vkQueueSubmit2(m_Device.queues.at(QueueFamily::graphics).handle, 1,
                   &queueSubmitInfo, VK_NULL_HANDLE);

    PendingSubmission submission{.cmdBuffer = cmdBuffer.m_CmdBuffer,
                                 .waitValue = m_SubmissionCount};

    cmdBuffer.reset();

    return {};
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

void SYN::RenderDevice::pollSubmissions() {
    while (!m_PendingSubmissions.empty()) {
        PendingSubmission submission{m_PendingSubmissions.front()};
        uint64_t currentValue{};
        vkGetSemaphoreCounterValue(m_Device.logical, m_CPUSubmissionSemaphore,
                                   &currentValue);

        if (currentValue >= submission.waitValue) {
            m_PendingSubmissions.pop();
            vkResetCommandBuffer(submission.cmdBuffer, 0);

            m_FreeCmdBuffers.emplace_back(submission.cmdBuffer);
        } else {
            break;
        }
    }
}

void SYN::RenderDevice::waitIdle() const { vkDeviceWaitIdle(m_Device.logical); }
