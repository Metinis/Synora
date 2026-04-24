#pragma once
#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/project/Assets.h"
#include "SynoraEngine/project/UUID.h"
#include "SynoraEngine/renderer/RenderTypes.h"

struct MeshData;

namespace SYN {
class Window;

class IGraphicsContext {
  public:
    IGraphicsContext() = default;
    virtual ~IGraphicsContext() = default;

    virtual void uploadToBuffer(BufferHandle handle, size_t size,
                                const void *data) = 0;

    virtual void uploadToTexture(TextureHandle handle,
                                 std::span<const void *> data, uint32_t width,
                                 uint32_t height) = 0;
    inline void uploadToTexture(TextureHandle handle, const void *data,
                                uint32_t width, uint32_t height) {
        uploadToTexture(handle, std::span(&data, 1), width, height);
    }

    // returns true if begin frame succeeded, false if otherwise
    virtual bool beginFrame(Window &window) = 0;
    // submits and presents frame
    virtual ReceiptHandle endFrame(Window &window) = 0;

    virtual void beginRenderPassCmd(const RenderPassDesc &desc,
                                    PipelineHandle pipeline) = 0;
    virtual void beginRenderPassCmd(const RenderPassDesc &desc,
                                    PipelineHandle pipeline,
                                    const void *uniformData,
                                    size_t uniformSize) = 0;
    template <typename T>
    void beginRenderPassCmd(const RenderPassDesc &desc, PipelineHandle pipeline,
                            const T &uniform) {
        beginRenderPassCmd(desc, pipeline, &uniform, sizeof(uniform));
    }

    virtual void endRenderPassCmd() = 0;

    virtual void setPushConstantsCmd(const void *data, size_t size) = 0;
    template <typename T> void setPushConstantsCmd(const T &pushConstants) {
        setPushConstantsCmd(&pushConstants, sizeof(pushConstants));
    }

    virtual void drawCmd(size_t nVertices) = 0;
    virtual void drawIndexedCmd(size_t nIndices) = 0;
    virtual void drawImGUI() = 0;

    virtual AttachmentHandle acquireSwapchainAttachmentCmd() = 0;

    // returns the index of the texture in the bindless sampler2D array that
    // is valid to access in the renderPass
    virtual uint32_t getShaderSamplerIndexCmd(TextureHandle texture) = 0;

    // only for attachments that are sampleable
    virtual uint32_t getShaderSamplerIndexCmd(AttachmentHandle attachment) = 0;
    virtual uint64_t getBufferAddressCmd(BufferHandle buffer) = 0;
};

} // namespace SYN
