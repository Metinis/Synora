#include "GraphicsContext.h"
#include "Device.h"

#include "core/Buffer.h"
#include "core/Commands.h"
#include "core/Device.h"
#include "core/Image.h"
#include "core/Pipeline.h"
#include "core/StagingBuffer.h"
#include "core/Swapchain.h"

#include <GLFW/glfw3.h>
#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <vk_mem_alloc.h>

#include <SynoraEngine/core/Window.h>
#include <vulkan/vulkan_core.h>

#include "imgui_impl_vulkan.h"

using namespace SYN;
using namespace SYN::VK;

namespace {
constexpr uint32_t c_MB{1024 * 1024};

VkRenderingAttachmentInfo makeAttachmentInfo(
    const Image &image, std::optional<const Image *> resolveImage,
    const WriteAttachmentInfo &attachment, VkImageLayout targetLayout);

VkAttachmentLoadOp toVkLoadOp(LoadOp loadOp);
VkAttachmentStoreOp toVkStoreOp(StoreOp loadOp);

} // namespace

SYN::VK::VulkanGraphicsContext::VulkanGraphicsContext(
    SYN::VK::VulkanDevice *device)
    : m_Device(device) {}

void SYN::VK::VulkanGraphicsContext::uploadToBuffer(BufferHandle handle,
                                                    size_t size,
                                                    const void *data) {
    const Buffer &buffer{m_Device->m_Buffers[handle]};
    m_Device->m_StagingBuffer.uploadToBuffer(m_Device->m_Device, data, size,
                                             buffer);
}

void SYN::VK::VulkanGraphicsContext::uploadToTexture(
    TextureHandle handle, std::span<const void *> data, uint32_t width,
    uint32_t height) {
    Texture &texture{m_Device->m_Textures[handle]};

    transitionImage(m_Device->m_TransientCmdPool, m_Device->m_Device,
                    texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT);

    m_Device->m_StagingBuffer.uploadToImage(m_Device->m_Device, data, width,
                                            height, texture.image);
    generateMipChain(m_Device->m_TransientCmdPool, m_Device->m_Device,
                     texture.image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                     VK_ACCESS_2_SHADER_READ_BIT);
}

void SYN::VK::VulkanGraphicsContext::beginFrame(Window &window) {
    m_Device->m_StagingBuffer.stallOnPendingUploads(m_Device->m_Device);

    FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};

    vkWaitForFences(m_Device->m_Device.logical, 1, &frame.renderFinishedFence,
                    VK_TRUE, UINT64_MAX);

    VkResult res{vkAcquireNextImageKHR(
        m_Device->m_Device.logical, m_Device->m_Swapchain.handle, UINT64_MAX,
        frame.imageAvailableSemaphore, VK_NULL_HANDLE,
        &frame.swapchainImageIndex)};

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        spdlog::info("Recreating swapchain");
        m_Device->recreateSwapchain(window);
        return;
    }

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        spdlog::error("Could not acquire swap chain image. VkResult = {}",
                      static_cast<int>(res));
    }

    Image swapchainImage{
        .handle = m_Device->m_Swapchain.images[frame.swapchainImageIndex],
        .view = m_Device->m_Swapchain.imageViews[frame.swapchainImageIndex],
        .extent = m_Device->m_Swapchain.extent,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
    };

    // TODO: add swapchain attachment to bindless descriptors and abstract
    // bindless descriptor system into its own thingy
    Attachment swapchainAttachment{
        .image = swapchainImage,
        .size = AttachmentSize::fixed,
    };

    frame.swapchainHandle = m_Device->m_Attachments.insert(swapchainAttachment);

    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkResetCommandPool(m_Device->m_Device.logical, frame.graphicsCmdPool, 0);
    vkBeginCommandBuffer(frame.graphicsCmdBuffer, &cmdBufferBeginInfo);

    VkDescriptorBufferInfo bufferInfo{
        .buffer = frame.UBOBuffer.getVkBuffer(),
        .range = DynamicUBO::c_MaxSizePerUBO,
    };

    VkWriteDescriptorSet writeDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_Device->m_UBODescriptorSet,
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .pBufferInfo = &bufferInfo,
    };
    vkUpdateDescriptorSets(m_Device->m_Device.logical, 1, &writeDescriptorSet,
                           0, nullptr);

    vkCmdBindDescriptorSets(frame.graphicsCmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_Device->m_GraphicsPipelineLayout, 0, 1,
                            &m_Device->m_BindlessDescriptorSet, 0, nullptr);
}

void SYN::VK::VulkanGraphicsContext::endFrame(Window &window) {
    FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};

    transitionImageCmd(frame.graphicsCmdBuffer,
                       m_Device->m_Attachments[frame.swapchainHandle].image,
                       VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                       VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE);

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
            &m_Device->m_RenderFinishedSemaphores[frame.swapchainImageIndex],
    };

    vkResetFences(m_Device->m_Device.logical, 1, &frame.renderFinishedFence);
    vkQueueSubmit(m_Device->m_Device.queues[QueueFamily::graphics].handle, 1,
                  &queueSubmitInfo, frame.renderFinishedFence);

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &m_Device->m_RenderFinishedSemaphores[frame.swapchainImageIndex],
        .swapchainCount = 1,
        .pSwapchains = &m_Device->m_Swapchain.handle,
        .pImageIndices = &frame.swapchainImageIndex,
    };

    VkResult res{vkQueuePresentKHR(
        m_Device->m_Device.queues[QueueFamily::present].handle, &presentInfo)};

    if ((res == VK_SUBOPTIMAL_KHR) || (res == VK_ERROR_OUT_OF_DATE_KHR)) {
        spdlog::info("Recreating swapchain");
        m_Device->recreateSwapchain(window);
    } else if (res != VK_SUCCESS) {
        spdlog::error("Could not present image. VkResult = {}",
                      static_cast<int>(res));
    }

    m_Device->m_Attachments.remove(frame.swapchainHandle);

    frame.UBOBuffer.reset();
    m_CurrentFrameIndex =
        (m_CurrentFrameIndex + 1) % m_Device->c_MaxFramesInFlight;
}

void SYN::VK::VulkanGraphicsContext::beginRenderPassCmd(
    const RenderPassDesc &desc, PipelineHandle pipelineHandle) {
    VkPipeline pipeline{m_Device->m_Pipelines[pipelineHandle]};

    FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};

    std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos;
    colorAttachmentInfos.reserve(desc.colorAttachments.size());
    VkRenderingAttachmentInfo depthAttachmentInfo{};

    VkExtent2D viewportExtent{.width = UINT32_MAX, .height = UINT32_MAX};
    for (const auto &attachment : desc.colorAttachments) {
        Image &image{m_Device->m_Attachments[attachment.handle].image};
        std::optional<Image *> resolveImage{};

        if (attachment.resolveHandle.has_value()) {
            resolveImage =
                &m_Device->m_Attachments[attachment.resolveHandle.value()]
                     .image;
        }

        if (viewportExtent.width == UINT32_MAX &&
            viewportExtent.height == UINT32_MAX) {
            viewportExtent = image.extent;
        }
        if (viewportExtent.width != image.extent.width ||
            viewportExtent.height != image.extent.height) {
            spdlog::warn(
                "Not all output attachments in renderpass are the same size");
            viewportExtent.height =
                std::max(viewportExtent.height, image.extent.height);
            viewportExtent.width =
                std::max(viewportExtent.width, image.extent.width);
        }

        VkImageLayout targetLayout{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorAttachmentInfos.emplace_back(
            makeAttachmentInfo(image, resolveImage, attachment, targetLayout));

        transitionImageCmd(frame.graphicsCmdBuffer, image, targetLayout,
                           VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                           VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

        if (resolveImage.has_value()) {
            transitionImageCmd(frame.graphicsCmdBuffer, *resolveImage.value(),
                               targetLayout,
                               VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                               VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        }
    }
    if (desc.depthAttachment.has_value()) {
        WriteAttachmentInfo attachment{desc.depthAttachment.value()};
        Image &image{m_Device->m_Attachments[attachment.handle].image};
        std::optional<Image *> resolveImage{};

        if (attachment.resolveHandle.has_value()) {
            resolveImage =
                &m_Device->m_Attachments[attachment.resolveHandle.value()]
                     .image;
        }

        if (viewportExtent.width == UINT32_MAX &&
            viewportExtent.height == UINT32_MAX) {
            viewportExtent = image.extent;
        }
        if (viewportExtent.width != image.extent.width ||
            viewportExtent.height != image.extent.height) {
            spdlog::warn(
                "Not all output attachments in renderpass are the same size");
            viewportExtent.height =
                std::max(viewportExtent.height, image.extent.height);
            viewportExtent.width =
                std::max(viewportExtent.width, image.extent.width);
        }
        VkImageLayout targetLayout{VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
        depthAttachmentInfo =
            makeAttachmentInfo(image, resolveImage, attachment, targetLayout);

        transitionImageCmd(frame.graphicsCmdBuffer, image, targetLayout,
                           VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                               VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                           VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT);
        if (resolveImage.has_value()) {
            transitionImageCmd(frame.graphicsCmdBuffer, *resolveImage.value(),
                               targetLayout,
                               VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                               VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);
        }
    }

    for (const auto &handle : desc.readAttachments) {
        Attachment &attachment{m_Device->m_Attachments[handle]};
        Image &image{attachment.image};
        VkImageLayout targetLayout{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        transitionImageCmd(frame.graphicsCmdBuffer, image, targetLayout,
                           VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                               VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                           VK_ACCESS_2_SHADER_SAMPLED_READ_BIT);
    }

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = VkRect2D{.extent = viewportExtent},
        .layerCount = 1,
        .colorAttachmentCount =
            static_cast<uint32_t>(colorAttachmentInfos.size()),
        .pColorAttachments = colorAttachmentInfos.data(),
    };
    if (desc.depthAttachment.has_value()) {
        renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    }

    vkCmdBeginRendering(frame.graphicsCmdBuffer, &renderingInfo);

    VkViewport viewport{.y = static_cast<float>(viewportExtent.height),
                        .width = static_cast<float>(viewportExtent.width),
                        .height = -static_cast<float>(viewportExtent.height),
                        .minDepth = 0.f,
                        .maxDepth = 1.f};

    VkRect2D scissor{.extent = VkExtent2D{
                         .width = viewportExtent.width,
                         .height = viewportExtent.height,
                     }};

    vkCmdBindPipeline(frame.graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      pipeline);

    vkCmdSetViewport(frame.graphicsCmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(frame.graphicsCmdBuffer, 0, 1, &scissor);
}

void SYN::VK::VulkanGraphicsContext::beginRenderPassCmd(
    const RenderPassDesc &desc, PipelineHandle pipelineHandle,
    const void *uniformData, size_t uniformSize) {
    FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};

    beginRenderPassCmd(desc, pipelineHandle);

    uint32_t dynamicOffset{frame.UBOBuffer.write(uniformData, uniformSize)};

    vkCmdBindDescriptorSets(frame.graphicsCmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_Device->m_GraphicsPipelineLayout, 1, 1,
                            &m_Device->m_UBODescriptorSet, 1, &dynamicOffset);
}
void SYN::VK::VulkanGraphicsContext::endRenderPassCmd() {
    const FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};
    vkCmdEndRendering(frame.graphicsCmdBuffer);
}

void SYN::VK::VulkanGraphicsContext::setPushConstantsCmd(const void *data,
                                                         size_t size) {
    const FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};
    if (size > m_Device->c_MinGuarenteedPushConstantSize) {
        spdlog::warn(
            "Push constant size is {} when the max size is {}, clamping", size,
            m_Device->c_MinGuarenteedPushConstantSize);
        size = m_Device->c_MinGuarenteedPushConstantSize;
    }

    vkCmdPushConstants(frame.graphicsCmdBuffer,
                       m_Device->m_GraphicsPipelineLayout, VK_SHADER_STAGE_ALL,
                       0, size, data);
}

void SYN::VK::VulkanGraphicsContext::drawCmd(size_t nVertices) {
    const FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};

    vkCmdDraw(frame.graphicsCmdBuffer, nVertices, 1, 0, 0);
}

void SYN::VK::VulkanGraphicsContext::drawIndexedCmd(size_t nIndices) {
    const FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};

    vkCmdDrawIndexed(frame.graphicsCmdBuffer, nIndices, 1, 0, 0, 0);
}

void VulkanGraphicsContext::drawImGUI() {
    const FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    frame.graphicsCmdBuffer);
}

AttachmentHandle
SYN::VK::VulkanGraphicsContext::acquireSwapchainAttachmentCmd() {
    const FrameData &frame{m_Device->m_FrameData[m_CurrentFrameIndex]};
    return frame.swapchainHandle;
}

uint32_t
SYN::VK::VulkanGraphicsContext::getShaderSamplerIndexCmd(TextureHandle handle) {
    return m_Device->m_Textures[handle].bindlessSamplerIndex;
}

uint32_t SYN::VK::VulkanGraphicsContext::getShaderSamplerIndexCmd(
    AttachmentHandle handle) {
    const Attachment &attachment{m_Device->m_Attachments[handle]};

    return attachment.bindlessSamplerIndex;
}

uint64_t
SYN::VK::VulkanGraphicsContext::getBufferAddressCmd(BufferHandle handle) {
    return m_Device->m_Buffers[handle].deviceAddress;
}

namespace {

VkRenderingAttachmentInfo makeAttachmentInfo(
    const Image &image, std::optional<const Image *> resolveImage,
    const WriteAttachmentInfo &attachment, VkImageLayout targetLayout) {

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

    if (resolveImage.has_value()) {
        assert(attachment.resolveHandle.has_value());

        attachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
        attachmentInfo.resolveImageLayout = targetLayout;
        attachmentInfo.resolveImageView = resolveImage.value()->view;
    }

    return attachmentInfo;
}
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

} // namespace
