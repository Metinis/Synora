#include "GraphicsCommandBuffer.h"
#include "../render_device/RenderDevice.h"

#include "imgui_impl_vulkan.h"
#include <vulkan/vulkan_core.h>

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

SYN::GraphicsCommandBuffer::GraphicsCommandBuffer(
    SYN::RenderDevice *renderDevice)
    : m_RenderDevice(renderDevice) {
    FrameData &frame{m_RenderDevice->getCurrentFrame()};

    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    vkBeginCommandBuffer(frame.graphicsCmdBuffer, &cmdBufferBeginInfo);

    vkCmdBindDescriptorSets(frame.graphicsCmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_RenderDevice->m_PipelineLayout, 0, 1,
                            &m_RenderDevice->m_BindlessSet, 0, nullptr);
    vkCmdBindDescriptorSets(frame.graphicsCmdBuffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            m_RenderDevice->m_PipelineLayout, 0, 1,
                            &m_RenderDevice->m_BindlessSet, 0, nullptr);
}

void SYN::GraphicsCommandBuffer::reset() { m_RenderDevice = nullptr; }

SYN::GraphicsCommandBuffer::~GraphicsCommandBuffer() {
    if (m_RenderDevice != nullptr) {
        spdlog::warn(
            "Graphics Command Buffer leaked; delete wasnt called on this "
            "before going out of scope");
    }
}

void SYN::GraphicsCommandBuffer::beginRenderPassCmd(
    const RenderPassDesc &desc, PipelineHandle pipelineHandle) {
    m_IsInRenderPass = true;

    VkPipeline pipeline{m_RenderDevice->m_Pipelines.at(pipelineHandle)};

    FrameData &frame{m_RenderDevice->getCurrentFrame()};

    std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos;
    colorAttachmentInfos.reserve(desc.colorAttachments.size());
    VkRenderingAttachmentInfo depthAttachmentInfo{};

    VkExtent2D viewportExtent{.width = UINT32_MAX, .height = UINT32_MAX};
    for (const auto &attachment : desc.colorAttachments) {
        Image &image{m_RenderDevice->m_Attachments[attachment.handle].image};
        std::optional<Image *> resolveImage{};

        if (attachment.resolveHandle.has_value()) {
            resolveImage =
                &m_RenderDevice->m_Attachments[attachment.resolveHandle.value()]
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
        Image &image{m_RenderDevice->m_Attachments[attachment.handle].image};
        std::optional<Image *> resolveImage{};

        if (attachment.resolveHandle.has_value()) {
            resolveImage =
                &m_RenderDevice->m_Attachments[attachment.resolveHandle.value()]
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
        Attachment &attachment{m_RenderDevice->m_Attachments[handle]};
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

void SYN::GraphicsCommandBuffer::beginRenderPassCmd(
    const RenderPassDesc &desc, PipelineHandle pipelineHandle,
    const void *uniformData, size_t uniformSize) {
    m_IsInRenderPass = true;

    FrameData &frame{m_RenderDevice->getCurrentFrame()};

    beginRenderPassCmd(desc, pipelineHandle);

    uint32_t dynamicOffset{frame.UBOBuffer.write(uniformData, uniformSize)};

    VkDescriptorSet uboDescriptorSet{m_RenderDevice->m_UBOSet};
    vkCmdBindDescriptorSets(frame.graphicsCmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            m_RenderDevice->m_PipelineLayout, 1, 1,
                            &uboDescriptorSet, 1, &dynamicOffset);
}
void SYN::GraphicsCommandBuffer::endRenderPassCmd() {
    m_IsInRenderPass = false;

    const FrameData &frame{m_RenderDevice->getCurrentFrame()};
    vkCmdEndRendering(frame.graphicsCmdBuffer);
}

void SYN::GraphicsCommandBuffer::setPushConstantsCmd(const void *data,
                                                     size_t size) {
    const FrameData &frame{m_RenderDevice->getCurrentFrame()};
    if (size > Limits::c_MinGuarenteedPushConstantSize) {
        spdlog::warn(
            "Push constant size is {} when the max size is {}, clamping", size,
            Limits::c_MinGuarenteedPushConstantSize);
        size = Limits::c_MinGuarenteedPushConstantSize;
    }

    vkCmdPushConstants(frame.graphicsCmdBuffer,
                       m_RenderDevice->m_PipelineLayout, VK_SHADER_STAGE_ALL, 0,
                       size, data);
}

void SYN::GraphicsCommandBuffer::dispatchCmd(const DispatchDesc &desc,
                                             PipelineHandle pipelineHandle) {
    if (m_IsInRenderPass) {
        spdlog::error(
            "Dispatching compute work within renderpass isn't allowed");
        assert(false);
    }

    const FrameData &frame{m_RenderDevice->getCurrentFrame()};

    assert(m_RenderDevice->m_Pipelines.contains(pipelineHandle));

    VkPipeline pipeline{m_RenderDevice->m_Pipelines[pipelineHandle]};
    vkCmdBindPipeline(frame.graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipeline);

    for (const auto &attachment : desc.readonlyAttachments) {
        Image &image{m_RenderDevice->m_Attachments[attachment].image};
        transitionImageCmd(frame.graphicsCmdBuffer, image,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                           VK_ACCESS_2_SHADER_READ_BIT);
    }
    for (const auto &attachment : desc.readWriteAttachments) {
        Image &image{m_RenderDevice->m_Attachments[attachment].image};
        transitionImageCmd(
            frame.graphicsCmdBuffer, image, VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT);
    }

    vkCmdDispatch(frame.graphicsCmdBuffer, desc.groupCountX, desc.groupCountY,
                  desc.groupCountZ);
}

void SYN::GraphicsCommandBuffer::drawCmd(size_t nVertices) {
    const FrameData &frame{m_RenderDevice->getCurrentFrame()};

    vkCmdDraw(frame.graphicsCmdBuffer, nVertices, 1, 0, 0);
}

void SYN::GraphicsCommandBuffer::drawIndexedCmd(size_t nIndices) {
    const FrameData &frame{m_RenderDevice->getCurrentFrame()};

    vkCmdDrawIndexed(frame.graphicsCmdBuffer, nIndices, 1, 0, 0, 0);
}

void GraphicsCommandBuffer::drawImGUI() {
    const FrameData &frame{m_RenderDevice->getCurrentFrame()};

    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),
                                    frame.graphicsCmdBuffer);
}

uint32_t
SYN::GraphicsCommandBuffer::getShaderTextureIndexCmd(TextureHandle handle) {
    return m_RenderDevice->m_Textures.at(handle).bindlessSamplerIndex;
}

uint32_t
SYN::GraphicsCommandBuffer::getShaderTextureIndexCmd(AttachmentHandle handle) {
    const Attachment &attachment{m_RenderDevice->m_Attachments.at(handle)};

    return attachment.bindlessTextureIndex;
}

uint64_t SYN::GraphicsCommandBuffer::getBufferAddressCmd(BufferHandle handle) {
    return m_RenderDevice->m_Buffers.at(handle).deviceAddress;
}

uint32_t SYN::GraphicsCommandBuffer::getShaderStorageImageIndexCmd(
    AttachmentHandle handle, uint32_t mipLevel) {
    assert(m_RenderDevice->m_Attachments.contains(handle));

    const Attachment &attachment{m_RenderDevice->m_Attachments.at(handle)};
    if (mipLevel > attachment.bindlessStorageImageIndices.size()) {
        spdlog::error("Trying to access shader storage image index for an "
                      "attachment that doesnt have one, or trying to access "
                      "mip level that attachment wasnt created with");
        assert(false);
    }

    return attachment.bindlessStorageImageIndices[mipLevel];
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
