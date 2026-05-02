#pragma once

#include "SynoraEngine/renderer/RenderTypes.h"

#include <stb_image.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct MeshData;
struct MeshComp;
struct VmaAllocator_T;

namespace SYN {

class RenderDevice;

class GraphicsCommandBuffer {
  private:
    // begins recording the cmd buffer
    GraphicsCommandBuffer(RenderDevice *renderDevice);

  public:
    ~GraphicsCommandBuffer();

    void beginRenderPassCmd(const RenderPassDesc &desc,
                            PipelineHandle pipeline);

    void beginRenderPassCmd(const RenderPassDesc &desc, PipelineHandle pipeline,
                            const void *uniformData, size_t uniformSize);
    template <typename T>
    void beginRenderPassCmd(const RenderPassDesc &desc, PipelineHandle pipeline,
                            const T &uniform);

    void endRenderPassCmd();

    void setPushConstantsCmd(const void *data, size_t size);
    template <typename T> void setPushConstantsCmd(const T &pushConstants);

    void dispatchCmd(const DispatchDesc &desc, PipelineHandle pipeline);

    void drawCmd(size_t nVertices);
    void drawIndexedCmd(size_t nIndices);
    void drawImGUI();

    uint32_t getShaderTextureIndexCmd(TextureHandle texture);
    uint32_t getShaderTextureIndexCmd(AttachmentHandle attachment);

    uint32_t getShaderStorageImageIndexCmd(AttachmentHandle attachment,
                                           uint32_t mipLevel = 0);

    uint64_t getBufferAddressCmd(BufferHandle buffer);

  private:
    friend RenderDevice;

    void reset();
    RenderDevice *m_RenderDevice;

    bool m_IsInRenderPass{false};
};

template <typename T>
void GraphicsCommandBuffer::beginRenderPassCmd(const RenderPassDesc &desc,
                                               PipelineHandle pipeline,
                                               const T &uniform) {
    beginRenderPassCmd(desc, pipeline, &uniform, sizeof(uniform));
}

template <typename T>
void GraphicsCommandBuffer::setPushConstantsCmd(const T &pushConstants) {
    setPushConstantsCmd(&pushConstants, sizeof(pushConstants));
}

} // namespace SYN
