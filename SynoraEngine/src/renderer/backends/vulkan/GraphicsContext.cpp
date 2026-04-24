#include "GraphicsContext.h"
#include "Limits.h"
#include "RenderDevice.h"

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
    SYN::VK::VulkanRenderDevice *renderDevice)
    : m_RenderDevice(renderDevice) {
    const Device &device{m_RenderDevice->getDevice()};
    const VmaAllocator &allocator{m_RenderDevice->getAllocator()};
    const Swapchain &swapchain{m_RenderDevice->getSwapchain()};

    VkCommandPoolCreateInfo transientCmdPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex =
            device.queues.at(QueueFamily::graphics).familyIndex};

    vkCreateCommandPool(device.logical, &transientCmdPoolCI, nullptr,
                        &m_TransientCmdPool);

    for (size_t i{}; i < c_MaxFramesInFlight; i++) {
        VkCommandPoolCreateInfo graphicsCmdPoolCI{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex =
                device.queues.at(QueueFamily::graphics).familyIndex};
        VkCommandPool graphicsCmdPool{};
        vkCreateCommandPool(device.logical, &graphicsCmdPoolCI, nullptr,
                            &graphicsCmdPool);

        VkCommandBufferAllocateInfo cmdBufferAllocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = graphicsCmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer graphicsCmdBuffer{};
        VkResult res{vkAllocateCommandBuffers(
            device.logical, &cmdBufferAllocInfo, &graphicsCmdBuffer)};
        if (res != VK_SUCCESS) {
            spdlog::error("Could not allocate command buffers, VkResult = {}",
                          static_cast<int>(res));
        }

        VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                  .flags = VK_FENCE_CREATE_SIGNALED_BIT};
        VkFence renderFinishedFence{};
        vkCreateFence(device.logical, &fenceCI, nullptr, &renderFinishedFence);

        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkSemaphore imageAvailableSemaphore{};
        vkCreateSemaphore(device.logical, &semaphoreCI, nullptr,
                          &imageAvailableSemaphore);

        DynamicUBO uboBuffer{};
        uboBuffer.create(device, allocator, 1 * c_MB);

        m_FrameData[i] =
            FrameData{.graphicsCmdPool = graphicsCmdPool,
                      .graphicsCmdBuffer = graphicsCmdBuffer,
                      .renderFinishedFence = renderFinishedFence,
                      .imageAvailableSemaphore = imageAvailableSemaphore,
                      .UBOBuffer = std::move(uboBuffer)};
    }
    for (const auto &_ : swapchain.images) {
        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkSemaphore renderFinishedSemaphore{};
        vkCreateSemaphore(device.logical, &semaphoreCI, nullptr,
                          &renderFinishedSemaphore);
        m_RenderFinishedSemaphores.emplace_back(renderFinishedSemaphore);
    }
}

SYN::VK::VulkanGraphicsContext::~VulkanGraphicsContext() {
    if (m_RenderDevice != nullptr) {
        spdlog::warn("Graphics Context leaked; delete wasnt called on this "
                     "graphics context before going out of scope");
    }
}

void SYN::VK::VulkanGraphicsContext::destroy() {
    const Device &device{m_RenderDevice->getDevice()};
    const VmaAllocator &allocator{m_RenderDevice->getAllocator()};

    vkDestroyCommandPool(device.logical, m_TransientCmdPool, nullptr);

    for (auto &frame : m_FrameData) {
        frame.UBOBuffer.destroy(allocator);
        vkDestroyFence(device.logical, frame.renderFinishedFence, nullptr);
        vkDestroySemaphore(device.logical, frame.imageAvailableSemaphore,
                           nullptr);
        vkDestroyCommandPool(device.logical, frame.graphicsCmdPool, nullptr);
        for (auto &buffer : frame.buffersToFree) {
            VK::destroyBuffer(allocator, buffer);
        }
        for (auto &image : frame.imagesToFree) {
            destroyImage(device, allocator, image);
        }
        for (auto &pipeline : frame.pipelinesToFree) {
            vkDestroyPipeline(device.logical, pipeline, nullptr);
        }
    }

    for (const auto &semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(device.logical, semaphore, nullptr);
    }

    m_RenderDevice = nullptr;
}

void SYN::VK::VulkanGraphicsContext::uploadToBuffer(BufferHandle handle,
                                                    size_t size,
                                                    const void *data) {
    const Device &device{m_RenderDevice->getDevice()};
    StagingBuffer &stagingBuffer{m_RenderDevice->getStagingBuffer()};

    const Buffer &buffer{m_RenderDevice->getBuffer(handle)};
    stagingBuffer.uploadToBuffer(device, data, size, buffer);
}

void SYN::VK::VulkanGraphicsContext::uploadToTexture(
    TextureHandle handle, std::span<const void *> data, uint32_t width,
    uint32_t height) {
    const Device &device{m_RenderDevice->getDevice()};
    StagingBuffer &stagingBuffer{m_RenderDevice->getStagingBuffer()};

    Texture &texture{m_RenderDevice->getTexture(handle)};

    transitionImage(m_TransientCmdPool, device, texture.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT);

    stagingBuffer.uploadToImage(device, data, width, height, texture.image);
    generateMipChain(m_TransientCmdPool, device, texture.image,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                     VK_ACCESS_2_SHADER_READ_BIT);
}

bool SYN::VK::VulkanGraphicsContext::beginFrame(Window &window) {
    const Device &device{m_RenderDevice->getDevice()};
    StagingBuffer &stagingBuffer{m_RenderDevice->getStagingBuffer()};
    const Swapchain &swapchain{m_RenderDevice->getSwapchain()};

    stagingBuffer.stallOnPendingUploads(device);

    FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    vkWaitForFences(device.logical, 1, &frame.renderFinishedFence, VK_TRUE,
                    UINT64_MAX);

    VkResult res{
        vkAcquireNextImageKHR(device.logical, swapchain.handle, UINT64_MAX,
                              frame.imageAvailableSemaphore, VK_NULL_HANDLE,
                              &frame.swapchainImageIndex)};

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        spdlog::info("Recreating swapchain");
        m_RenderDevice->recreateSwapchain(window);
        return false;
    }

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        spdlog::error("Could not acquire swap chain image. VkResult = {}",
                      static_cast<int>(res));
        return false;
    }

    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkResetCommandPool(device.logical, frame.graphicsCmdPool, 0);
    vkBeginCommandBuffer(frame.graphicsCmdBuffer, &cmdBufferBeginInfo);

    VkDescriptorBufferInfo bufferInfo{
        .buffer = frame.UBOBuffer.getVkBuffer(),
        .range = DynamicUBO::c_MaxSizePerUBO,
    };

    VkWriteDescriptorSet writeDescriptorSet{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_RenderDevice->getUBODescriptorSet(),
        .dstBinding = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .pBufferInfo = &bufferInfo,
    };
    vkUpdateDescriptorSets(device.logical, 1, &writeDescriptorSet, 0, nullptr);

    VkDescriptorSet bindlessDescriptorSet{
        m_RenderDevice->getBindlessDescriptorSet()};

    vkCmdBindDescriptorSets(frame.graphicsCmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_RenderDevice->getGraphicsPipelineLayout(), 0, 1,
                            &bindlessDescriptorSet, 0, nullptr);

    return true;
}

ReceiptHandle SYN::VK::VulkanGraphicsContext::endFrame(Window &window) {
    const Device &device{m_RenderDevice->getDevice()};
    FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    AttachmentHandle swapchainAttachmentHandle{
        m_RenderDevice->getSwapchainAttachmentHandle(
            frame.swapchainImageIndex)};

    transitionImageCmd(
        frame.graphicsCmdBuffer,
        m_RenderDevice->getAttachment(swapchainAttachmentHandle).image,
        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        VK_ACCESS_2_NONE);

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

    vkResetFences(device.logical, 1, &frame.renderFinishedFence);
    vkQueueSubmit(device.queues.at(QueueFamily::graphics).handle, 1,
                  &queueSubmitInfo, frame.renderFinishedFence);

    VkSwapchainKHR swapchainHandle{m_RenderDevice->getSwapchain().handle};
    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &m_RenderFinishedSemaphores[frame.swapchainImageIndex],
        .swapchainCount = 1,
        .pSwapchains = &swapchainHandle,
        .pImageIndices = &frame.swapchainImageIndex,
    };

    VkResult res{vkQueuePresentKHR(
        device.queues.at(QueueFamily::present).handle, &presentInfo)};

    if ((res == VK_SUBOPTIMAL_KHR) || (res == VK_ERROR_OUT_OF_DATE_KHR)) {
        spdlog::info("Recreating swapchain");
        m_RenderDevice->recreateSwapchain(window);
    } else if (res != VK_SUCCESS) {
        spdlog::error("Could not present image. VkResult = {}",
                      static_cast<int>(res));
    }

    frame.UBOBuffer.reset();
    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % c_MaxFramesInFlight;

    return {};
}

void SYN::VK::VulkanGraphicsContext::beginRenderPassCmd(
    const RenderPassDesc &desc, PipelineHandle pipelineHandle) {
    VkPipeline pipeline{m_RenderDevice->getPipeline(pipelineHandle)};

    FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos;
    colorAttachmentInfos.reserve(desc.colorAttachments.size());
    VkRenderingAttachmentInfo depthAttachmentInfo{};

    VkExtent2D viewportExtent{.width = UINT32_MAX, .height = UINT32_MAX};
    for (const auto &attachment : desc.colorAttachments) {
        Image &image{m_RenderDevice->getAttachment(attachment.handle).image};
        std::optional<Image *> resolveImage{};

        if (attachment.resolveHandle.has_value()) {
            resolveImage =
                &m_RenderDevice->getAttachment(attachment.resolveHandle.value())
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
        Image &image{m_RenderDevice->getAttachment(attachment.handle).image};
        std::optional<Image *> resolveImage{};

        if (attachment.resolveHandle.has_value()) {
            resolveImage =
                &m_RenderDevice->getAttachment(attachment.resolveHandle.value())
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
        Attachment &attachment{m_RenderDevice->getAttachment(handle)};
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
    FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    beginRenderPassCmd(desc, pipelineHandle);

    uint32_t dynamicOffset{frame.UBOBuffer.write(uniformData, uniformSize)};

    VkDescriptorSet uboDescriptorSet{m_RenderDevice->getUBODescriptorSet()};
    vkCmdBindDescriptorSets(frame.graphicsCmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_RenderDevice->getGraphicsPipelineLayout(), 1, 1,
                            &uboDescriptorSet, 1, &dynamicOffset);
}
void SYN::VK::VulkanGraphicsContext::endRenderPassCmd() {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    vkCmdEndRendering(frame.graphicsCmdBuffer);
}

void SYN::VK::VulkanGraphicsContext::setPushConstantsCmd(const void *data,
                                                         size_t size) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    if (size > Limits::c_MinGuarenteedPushConstantSize) {
        spdlog::warn(
            "Push constant size is {} when the max size is {}, clamping", size,
            Limits::c_MinGuarenteedPushConstantSize);
        size = Limits::c_MinGuarenteedPushConstantSize;
    }

    vkCmdPushConstants(frame.graphicsCmdBuffer,
                       m_RenderDevice->getGraphicsPipelineLayout(),
                       VK_SHADER_STAGE_ALL, 0, size, data);
}

void SYN::VK::VulkanGraphicsContext::drawCmd(size_t nVertices) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    vkCmdDraw(frame.graphicsCmdBuffer, nVertices, 1, 0, 0);
}

void SYN::VK::VulkanGraphicsContext::drawIndexedCmd(size_t nIndices) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    vkCmdDrawIndexed(frame.graphicsCmdBuffer, nIndices, 1, 0, 0, 0);
}

void VulkanGraphicsContext::drawImGUI() {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    frame.graphicsCmdBuffer);
}

AttachmentHandle
SYN::VK::VulkanGraphicsContext::acquireSwapchainAttachmentCmd() {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    return m_RenderDevice->getSwapchainAttachmentHandle(
        frame.swapchainImageIndex);
}

uint32_t
SYN::VK::VulkanGraphicsContext::getShaderSamplerIndexCmd(TextureHandle handle) {
    return m_RenderDevice->getTexture(handle).bindlessSamplerIndex;
}

uint32_t SYN::VK::VulkanGraphicsContext::getShaderSamplerIndexCmd(
    AttachmentHandle handle) {
    const Attachment &attachment{m_RenderDevice->getAttachment(handle)};

    return attachment.bindlessSamplerIndex;
}

uint64_t
SYN::VK::VulkanGraphicsContext::getBufferAddressCmd(BufferHandle handle) {
    return m_RenderDevice->getBuffer(handle).deviceAddress;
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
