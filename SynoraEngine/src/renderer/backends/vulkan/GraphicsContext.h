#pragma once

#include "SynoraEngine/renderer/RenderTypes.h"
#include "core/Buffer.h"
#include "core/Device.h"
#include "core/DynamicUBO.h"
#include "core/Image.h"
#include "core/SlotMap.h"
#include "core/StagingBuffer.h"
#include "core/Swapchain.h"
#include "renderer/backends/IGraphicsContext.h"

#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct MeshData;
struct MeshComp;
struct VmaAllocator_T;

namespace SYN::VK {

class VulkanDevice;

class VulkanGraphicsContext : public IGraphicsContext {
  public:
    VulkanGraphicsContext(VulkanDevice *device);
    ~VulkanGraphicsContext() = default;

    void uploadToBuffer(BufferHandle handle, size_t size,
                        const void *data) override;

    void uploadToTexture(TextureHandle handle, std::span<const void *> data,
                         uint32_t width, uint32_t height) override;

    void beginFrame(Window &window) override;
    void endFrame(Window &window) override;

    void beginRenderPassCmd(const RenderPassDesc &desc,
                            PipelineHandle pipeline) override;
    void beginRenderPassCmd(const RenderPassDesc &desc, PipelineHandle pipeline,
                            const void *uniformData,
                            size_t uniformSize) override;
    void endRenderPassCmd() override;

    void setPushConstantsCmd(const void *data, size_t size) override;

    void drawCmd(size_t nVertices) override;
    void drawIndexedCmd(size_t nIndices) override;
    void drawImGUI() override;

    AttachmentHandle acquireSwapchainAttachmentCmd() override;

    uint32_t getShaderSamplerIndexCmd(TextureHandle texture) override;
    uint32_t getShaderSamplerIndexCmd(AttachmentHandle attachment) override;

    uint64_t getBufferAddressCmd(BufferHandle buffer) override;

  private:
    VulkanDevice *m_Device;
    uint32_t m_CurrentFrameIndex{};
};

} // namespace SYN::VK
