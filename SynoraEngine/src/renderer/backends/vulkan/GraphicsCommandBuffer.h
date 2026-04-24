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
    GraphicsCommandBuffer(RenderDevice *renderDevice);

  public:
    ~GraphicsCommandBuffer();

    void beginRecording();
    void endRecording();

    void beginRenderPassCmd(const RenderPassDesc &desc,
                            PipelineHandle pipeline);

    void beginRenderPassCmd(const RenderPassDesc &desc, PipelineHandle pipeline,
                            const void *uniformData, size_t uniformSize);
    template <typename T>
    void beginRenderPassCmd(const RenderPassDesc &desc, PipelineHandle pipeline,
                            const T &uniform) {
        beginRenderPassCmd(desc, pipeline, &uniform, sizeof(uniform));
    }

    void endRenderPassCmd();

    void setPushConstantsCmd(const void *data, size_t size);
    template <typename T> void setPushConstantsCmd(const T &pushConstants) {
        setPushConstantsCmd(&pushConstants, sizeof(pushConstants));
    }

    void drawCmd(size_t nVertices);
    void drawIndexedCmd(size_t nIndices);
    void drawImGUI();

    uint32_t getShaderTextureIndexCmd(TextureHandle texture);
    uint32_t getShaderTextureIndexCmd(AttachmentHandle attachment);

    uint64_t getBufferAddressCmd(BufferHandle buffer);

  private:
    friend RenderDevice;

    RenderDevice *m_RenderDevice;
};

} // namespace SYN
