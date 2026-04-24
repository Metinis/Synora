#include "UploadCommandBuffer.h"
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
#include "renderer/backends/vulkan/UploadCommandBuffer.h"

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
