#include "UploadCommandBuffer.h"
#include "../core/Image.h"
#include "../render_device/RenderDevice.h"
#include "renderer/backends/vulkan/core/Commands.h"
#include "renderer/backends/vulkan/core/Image.h"
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

SYN::UploadCommandBuffer::UploadCommandBuffer(SYN::RenderDevice *renderDevice)
    : m_RenderDevice(renderDevice) {}

SYN::UploadCommandBuffer::~UploadCommandBuffer() {
    if (m_RenderDevice != nullptr) {
        spdlog::warn(
            "Upload Command Buffer leaked; delete wasnt called on this "
            "before going out of scope");
    }
}

void SYN::UploadCommandBuffer::reset() { m_RenderDevice = nullptr; }

// TODO: make this correctly batch stuff / defer actually uploading
void SYN::UploadCommandBuffer::uploadToBuffer(BufferHandle handle, size_t size,
                                              const void *data) {
    const Device &device{m_RenderDevice->m_Device};
    StagingBuffer &stagingBuffer{m_RenderDevice->m_StagingBuffer};

    const Buffer &buffer{m_RenderDevice->m_Buffers[handle]};
    stagingBuffer.uploadToBuffer(device, data, size, buffer);
}

void SYN::UploadCommandBuffer::uploadToTexture(TextureHandle handle,
                                               std::span<const void *> data,
                                               uint32_t width,
                                               uint32_t height) {
    const Device &device{m_RenderDevice->m_Device};
    StagingBuffer &stagingBuffer{m_RenderDevice->m_StagingBuffer};

    Texture &texture{m_RenderDevice->m_Textures[handle]};

    transitionImage(m_RenderDevice->m_TransientCmdPool, device, texture.image,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    VK_ACCESS_2_TRANSFER_WRITE_BIT);

    stagingBuffer.uploadToImage(device, data, width, height, texture.image);
    generateMipChain(m_RenderDevice->m_TransientCmdPool, device, texture.image,
                     VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                     VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                     VK_ACCESS_2_SHADER_READ_BIT);
}

void SYN::UploadCommandBuffer::copyAttachmentToBuffer(
    AttachmentHandle srcAttachmentHandle, BufferHandle dstBufferHandle,
    uint32_t mipLevel, uint32_t arrayLayer) {
    Image &image{m_RenderDevice->m_Attachments[srcAttachmentHandle].image};

    assert(image.mipLevels > mipLevel);
    assert(image.layerCount > arrayLayer);

    Buffer &buffer{m_RenderDevice->m_Buffers[dstBufferHandle]};

    VkCommandBuffer cmdBuf{beginTransientCmd(m_RenderDevice->m_TransientCmdPool,
                                             m_RenderDevice->m_Device)};

    VkBufferImageCopy region{
        .imageSubresource =
            {
                .aspectMask = image.subresourceRange.aspectMask,
                .mipLevel = mipLevel,
                .baseArrayLayer = arrayLayer,
                .layerCount = 1,
            },
        .imageExtent =
            {
                .width = image.extent.width,
                .height = image.extent.height,
                .depth = 1,
            },
    };

    transitionImageCmd(cmdBuf, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE);

    vkCmdCopyImageToBuffer(cmdBuf, image.handle, image.syncState.currentLayout,
                           buffer.handle, 1, &region);

    endTransientCmd(cmdBuf);
    submitCmdBuffer(
        m_RenderDevice->m_TransientCmdPool, cmdBuf, m_RenderDevice->m_Device,
        m_RenderDevice->m_Device.queues.at(QueueFamily::transfer).handle);
}
