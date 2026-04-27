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

class ComputeCommandBuffer {
  private:
    // begins recording the cmd buffer
    ComputeCommandBuffer(RenderDevice *renderDevice);

  public:
    ~ComputeCommandBuffer();

    void bindPipelineCmd(PipelineHandle pipelineHandle);
    void dispatchCmd(const DispatchDesc &desc);

    void setPushConstantsCmd(const void *data, size_t size);
    template <typename T> void setPushConstantsCmd(const T &pushConstants) {
        setPushConstantsCmd(&pushConstants, sizeof(pushConstants));
    }

    uint32_t getShaderTextureIndexCmd(TextureHandle texture);
    uint32_t getShaderTextureIndexCmd(AttachmentHandle attachment);

    uint32_t getShaderStorageImageCmd(AttachmentHandle attachment);

    uint64_t getBufferAddressCmd(BufferHandle buffer);

  private:
    friend RenderDevice;

    void reset();

    RenderDevice *m_RenderDevice;
    VkCommandBuffer m_CmdBuffer;
};

} // namespace SYN
