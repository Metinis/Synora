#include "ComputeCommandBuffer.h"
#include "../render_device/RenderDevice.h"
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

SYN::ComputeCommandBuffer::ComputeCommandBuffer(SYN::RenderDevice *renderDevice)
    : m_RenderDevice(renderDevice) {
    assert(renderDevice != nullptr);

    m_CmdBuffer = m_RenderDevice->acquireCommandBuffer();

    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};

    vkBeginCommandBuffer(m_CmdBuffer, &cmdBufferBeginInfo);

    vkCmdBindDescriptorSets(m_CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            m_RenderDevice->m_PipelineLayout, 0, 1,
                            &m_RenderDevice->m_BindlessSet, 0, nullptr);
}

void SYN::ComputeCommandBuffer::reset() {
    m_RenderDevice = nullptr;
    m_CmdBuffer = VK_NULL_HANDLE;
}

SYN::ComputeCommandBuffer::~ComputeCommandBuffer() {
    if (m_RenderDevice != nullptr) {
        spdlog::warn(
            "Compute Command Buffer leaked; reset wasnt called on this "
            "before going out of scope");
    }
}

void SYN::ComputeCommandBuffer::setPushConstantsCmd(const void *data,
                                                    size_t size) {
    if (size > Limits::c_MinGuarenteedPushConstantSize) {
        spdlog::warn(
            "Push constant size is {} when the max size is {}, clamping", size,
            Limits::c_MinGuarenteedPushConstantSize);
        size = Limits::c_MinGuarenteedPushConstantSize;
    }

    vkCmdPushConstants(m_CmdBuffer, m_RenderDevice->m_PipelineLayout,
                       VK_SHADER_STAGE_ALL, 0, size, data);
}

void SYN::ComputeCommandBuffer::dispatchCmd(const DispatchDesc &desc,
                                            PipelineHandle pipelineHandle) {
    assert(m_RenderDevice->m_Pipelines.contains(pipelineHandle));

    VkPipeline pipeline{m_RenderDevice->m_Pipelines[pipelineHandle]};
    vkCmdBindPipeline(m_CmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

    for (const auto &attachment : desc.readonlyAttachments) {
        Image &image{m_RenderDevice->m_Attachments[attachment].image};
        transitionImageCmd(m_CmdBuffer, image,
                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                           VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                           VK_ACCESS_2_SHADER_READ_BIT);
    }
    for (const auto &attachment : desc.readWriteAttachments) {
        Image &image{m_RenderDevice->m_Attachments[attachment].image};
        transitionImageCmd(m_CmdBuffer, image, VK_IMAGE_LAYOUT_GENERAL,
                           VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                           VK_ACCESS_2_SHADER_READ_BIT |
                               VK_ACCESS_2_SHADER_WRITE_BIT);
    }
    for (const auto &attachment : desc.writeonlyAttachments) {
        Image &image{m_RenderDevice->m_Attachments[attachment].image};
        transitionImageCmd(m_CmdBuffer, image, VK_IMAGE_LAYOUT_GENERAL,
                           VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                           VK_ACCESS_2_SHADER_WRITE_BIT);
    }

    vkCmdDispatch(m_CmdBuffer, desc.groupCountX, desc.groupCountY,
                  desc.groupCountZ);
}

uint32_t
SYN::ComputeCommandBuffer::getShaderTextureIndexCmd(TextureHandle handle) {
    assert(m_RenderDevice->m_Textures.contains(handle));
    return m_RenderDevice->m_Textures.at(handle).bindlessSamplerIndex;
}

uint32_t
SYN::ComputeCommandBuffer::getShaderTextureIndexCmd(AttachmentHandle handle) {
    assert(m_RenderDevice->m_Attachments.contains(handle));
    const Attachment &attachment{m_RenderDevice->m_Attachments.at(handle)};

    return attachment.bindlessTextureIndex;
}

uint32_t SYN::ComputeCommandBuffer::getShaderStorageImageIndexCmd(
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

uint64_t SYN::ComputeCommandBuffer::getBufferAddressCmd(BufferHandle handle) {
    assert(m_RenderDevice->m_Buffers.contains(handle));
    return m_RenderDevice->m_Buffers.at(handle).deviceAddress;
}
