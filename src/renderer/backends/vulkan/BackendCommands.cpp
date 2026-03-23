#include "Backend.h"
#include "core/Buffer.h"
#include "core/Commands.h"
#include "core/Device.h"
#include "core/Image.h"
#include "core/Pipeline.h"
#include "core/StagingBuffer.h"
#include "core/Swapchain.h"

#include <GLFW/glfw3.h>
#include <cstring>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <vk_mem_alloc.h>

#include <PuzzleEngine/core/Window.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

namespace {
constexpr uint32_t c_MB{1024 * 1024};

VkAttachmentLoadOp toVkLoadOp(LoadOp loadOp);
VkAttachmentStoreOp toVkStoreOp(StoreOp loadOp);

VkRenderingAttachmentInfo makeAttachmentInfo(const Image &image,
                                             const WriteAttachment &attachment,
                                             VkImageLayout targetLayout);

struct StageAccess {
    VkPipelineStageFlags2 stageMask;
    VkAccessFlags2 accessMask;
};

enum AccessMask : uint32_t {
    ACCESS_WRITE_BIT = 0b1,
    ACCESS_READ_BIT = 0b10,
};

StageAccess getStageAccess(VkImageLayout layout);

VkImageMemoryBarrier2 makeImageMemoryBarrier(const Image &image,
                                             VkImageLayout targetLayout,
                                             uint32_t srcQueueFamilyIndex = 0,
                                             uint32_t dstQueueFamilyIndex = 0);

struct PushConstants {
    VkDeviceAddress vertexBuffer;
};

} // namespace

void SYN::VK::VulkanBackend::beginFrame(Window &window) {
    m_StagingBuffer.stallOnPendingUploads(m_Device);
    FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    vkWaitForFences(m_Device.logical, 1, &frame.renderFinishedFence, VK_TRUE,
                    UINT64_MAX);

    VkResult res{
        vkAcquireNextImageKHR(m_Device.logical, m_Swapchain.handle, UINT64_MAX,
                              frame.imageAvailableSemaphore, VK_NULL_HANDLE,
                              &frame.swapchainImageIndex)};

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        spdlog::info("Recreating swapchain");
        recreateSwapchain(window);
        return;
    }

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        spdlog::error("Could not acquire swap chain image. VkResult = {}",
                      static_cast<int>(res));
    }

    Image swapchainImage{
        .handle = m_Swapchain.images[frame.swapchainImageIndex],
        .view = m_Swapchain.imageViews[frame.swapchainImageIndex],
        .extent = m_Swapchain.extent,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        .currentLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    if (!m_Attachments.contains(m_SwapchainAttachmentHandle)) {
        spdlog::warn("Swapchain was not properly added to list of attachments");
        m_SwapchainAttachmentHandle = m_Attachments.insert({});
    }
    m_Attachments[m_SwapchainAttachmentHandle][m_CurrentFrameIndex] =
        swapchainImage;

    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkResetCommandPool(m_Device.logical, frame.graphicsCmdPool, 0);
    vkBeginCommandBuffer(frame.graphicsCmdBuffer, &cmdBufferBeginInfo);
    vkCmdBindDescriptorSets(frame.graphicsCmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
                            0, 1, &m_BindlessDescriptorSet, 0, nullptr);
}

void SYN::VK::VulkanBackend::endFrame(Window &window) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    VkImageMemoryBarrier2 presentBarrier{makeImageMemoryBarrier(
        m_Attachments[m_SwapchainAttachmentHandle][m_CurrentFrameIndex],
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_ACCESS_2_NONE)};

    VkDependencyInfo presentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &presentBarrier,
    };
    vkCmdPipelineBarrier2(frame.graphicsCmdBuffer, &presentDependency);

    vkEndCommandBuffer(frame.graphicsCmdBuffer);

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
            &m_RenderFinishedSemaphores[frame.swapchainImageIndex],
    };

    vkResetFences(m_Device.logical, 1, &frame.renderFinishedFence);
    vkQueueSubmit(m_Device.queues[QueueFamily::graphics].handle, 1,
                  &queueSubmitInfo, frame.renderFinishedFence);

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &m_RenderFinishedSemaphores[frame.swapchainImageIndex],
        .swapchainCount = 1,
        .pSwapchains = &m_Swapchain.handle,
        .pImageIndices = &frame.swapchainImageIndex,
    };

    VkResult res{vkQueuePresentKHR(m_Device.queues[QueueFamily::present].handle,
                                   &presentInfo)};

    if ((res == VK_SUBOPTIMAL_KHR) || (res == VK_ERROR_OUT_OF_DATE_KHR)) {
        spdlog::info("Recreating swapchain");
        recreateSwapchain(window);
    } else if (res != VK_SUCCESS) {
        spdlog::error("Could not present image. VkResult = {}",
                      static_cast<int>(res));
    }

    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % c_MaxFramesInFlight;
}

RenderPassHandle
SYN::VK::VulkanBackend::beginRenderPassCmd(const RenderPassDesc &desc) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    std::vector<VkImageMemoryBarrier2> attachmentBarriers{};

    std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos;
    colorAttachmentInfos.reserve(desc.colorAttachments.size());
    VkRenderingAttachmentInfo depthAttachmentInfo{};

    for (const auto &attachment : desc.colorAttachments) {
        Image &image{
            m_Attachments[attachment.textureHandle][m_CurrentFrameIndex]};
        VkImageLayout targetLayout{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorAttachmentInfos.emplace_back(
            makeAttachmentInfo(image, attachment, targetLayout));

        if (image.currentLayout != targetLayout) {
            attachmentBarriers.emplace_back(makeImageMemoryBarrier(
                image, targetLayout,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT));
            image.currentLayout = targetLayout;
        }
    }
    if (desc.depthAttachment.has_value()) {
        WriteAttachment attachment{desc.depthAttachment.value()};
        Image &image{
            m_Attachments[attachment.textureHandle][m_CurrentFrameIndex]};
        VkImageLayout targetLayout{VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
        depthAttachmentInfo =
            makeAttachmentInfo(image, attachment, targetLayout);

        if (image.currentLayout != targetLayout) {

            attachmentBarriers.emplace_back(makeImageMemoryBarrier(
                image, targetLayout,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                    VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT));
            image.currentLayout = targetLayout;
        }
    }

    for (const auto &handle : desc.readAttachments) {
        Image &image{m_Attachments[handle][m_CurrentFrameIndex]};
        VkImageLayout targetLayout{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        if (image.currentLayout != targetLayout) {
            attachmentBarriers.emplace_back(makeImageMemoryBarrier(
                image, targetLayout,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                VK_ACCESS_2_SHADER_SAMPLED_READ_BIT));
            image.currentLayout = targetLayout;
        }
    }

    VkDependencyInfo colorAttachmentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount =
            static_cast<uint32_t>(attachmentBarriers.size()),
        .pImageMemoryBarriers = attachmentBarriers.data(),
    };

    vkCmdPipelineBarrier2(frame.graphicsCmdBuffer, &colorAttachmentDependency);

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = VkRect2D{.extent =
                                   VkExtent2D{
                                       .width = desc.viewport.width,
                                       .height = desc.viewport.height,
                                   }},
        .layerCount = 1,
        .colorAttachmentCount =
            static_cast<uint32_t>(colorAttachmentInfos.size()),
        .pColorAttachments = colorAttachmentInfos.data(),
    };
    if (desc.depthAttachment.has_value()) {
        renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    }

    vkCmdBeginRendering(frame.graphicsCmdBuffer, &renderingInfo);

    VkViewport viewport{.y = static_cast<float>(desc.viewport.height),
                        .width = static_cast<float>(desc.viewport.width),
                        .height = -static_cast<float>(desc.viewport.height),
                        .minDepth = 0.f,
                        .maxDepth = 1.f};

    VkRect2D scissor{.extent = VkExtent2D{
                         .width = desc.viewport.width,
                         .height = desc.viewport.height,
                     }};

    vkCmdSetViewport(frame.graphicsCmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(frame.graphicsCmdBuffer, 0, 1, &scissor);
    // TODO: add a way to create pipelines and use those instead
    vkCmdBindPipeline(frame.graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_GraphicsPipeline);
    // TODO: later on maybe we wanna add some per render pass stuff?
    return {};
}
void SYN::VK::VulkanBackend::endRenderPassCmd(RenderPassHandle &renderPass) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    vkCmdEndRendering(frame.graphicsCmdBuffer);
}

void SYN::VK::VulkanBackend::drawCmd(BufferHandle vertexBufferHandle,
                                     size_t nVertices) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    Buffer vertexBuffer{m_Buffers[vertexBufferHandle]};
    PushConstants pushConstants{.vertexBuffer = vertexBuffer.deviceAddress};

    vkCmdPushConstants(frame.graphicsCmdBuffer, m_PipelineLayout,
                       VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants),
                       &pushConstants);

    vkCmdDraw(frame.graphicsCmdBuffer, nVertices, 1, 0, 0);
}

TextureHandle SYN::VK::VulkanBackend::getSwapchainTextureCmd() {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    return m_SwapchainAttachmentHandle;
}

Viewport SYN::VK::VulkanBackend::getSwapchainViewport() {
    return Viewport{.width = m_Swapchain.extent.width,
                    .height = m_Swapchain.extent.height};
}

namespace {
VkAttachmentLoadOp toVkLoadOp(LoadOp loadOp) {
    VkAttachmentLoadOp vkLoadOp{};
    switch (loadOp) {
    case LoadOp::clear:
        vkLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        break;
    case LoadOp::load:
        vkLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        break;
    case LoadOp::dontCare:
        vkLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        break;
    default:
        break;
    }
    return vkLoadOp;
}
VkAttachmentStoreOp toVkStoreOp(StoreOp storeOp) {
    VkAttachmentStoreOp vkStoreOp{};
    switch (storeOp) {
    case StoreOp::store:
        vkStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        break;
    case StoreOp::dontCare:
        vkStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        break;
    default:
        break;
    }
    return vkStoreOp;
}

VkRenderingAttachmentInfo makeAttachmentInfo(const Image &image,
                                             const WriteAttachment &attachment,
                                             VkImageLayout targetLayout) {
    VkClearValue clearValue{};
    if (image.subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
        clearValue = {.color = VkClearColorValue{
                          attachment.clearColor.r,
                          attachment.clearColor.g,
                          attachment.clearColor.b,
                          attachment.clearColor.a,
                      }};
    } else {
        clearValue = {.depthStencil = VkClearDepthStencilValue{
                          .depth = attachment.clearDepth}};
    }

    VkAttachmentLoadOp loadOp{toVkLoadOp(attachment.loadOp)};
    VkAttachmentStoreOp storeOp{toVkStoreOp(attachment.storeOp)};

    VkRenderingAttachmentInfo attachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = image.view,
        .imageLayout = targetLayout,
        .loadOp = loadOp,
        .storeOp = storeOp,
        .clearValue = clearValue,
    };

    return attachmentInfo;
}

StageAccess getStageAccess(VkImageLayout layout) {
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return {VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE};
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_GENERAL:
        return {};
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                VK_ACCESS_2_SHADER_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT};
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return {VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE};
    default:
        spdlog::warn("Unspecified layout encountered");
        assert(false);
        return {VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT};
    }
}

VkImageMemoryBarrier2 makeImageMemoryBarrier(const Image &image,
                                             VkImageLayout targetLayout,
                                             uint32_t srcQueueFamilyIndex,
                                             uint32_t dstQueueFamilyIndex) {
    StageAccess srcStageAccess{getStageAccess(image.currentLayout)};
    StageAccess dstStageAccess{getStageAccess(targetLayout)};
    return VkImageMemoryBarrier2{.sType =
                                     VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                 .srcStageMask = srcStageAccess.stageMask,
                                 .srcAccessMask = srcStageAccess.accessMask,
                                 .dstStageMask = dstStageAccess.stageMask,
                                 .dstAccessMask = dstStageAccess.accessMask,
                                 .oldLayout = image.currentLayout,
                                 .newLayout = targetLayout,
                                 .srcQueueFamilyIndex = srcQueueFamilyIndex,
                                 .dstQueueFamilyIndex = dstQueueFamilyIndex,
                                 .image = image.handle,
                                 .subresourceRange = image.subresourceRange};
}
} // namespace
