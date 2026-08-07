#pragma once
#include "../RenderTypes.h"
#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/project/Assets.h"
#include "SynoraEngine/project/UUID.h"

struct MeshData;

namespace SYN {
class Window;

class IBackend {
  public:
    IBackend() = default;
    virtual ~IBackend() = default;

    virtual void init(Window *window) = 0;
    virtual BufferHandle createBuffer(const BufferDesc &desc) = 0;
    virtual void uploadToBuffer(BufferHandle handle, size_t size,
                                const void *data) = 0;
    inline BufferHandle uploadBuffer(const BufferDesc &desc, const void *data) {
        BufferHandle handle{createBuffer(desc)};
        uploadToBuffer(handle, desc.size, data);
        return handle;
    }

    virtual void destroyBuffer(BufferHandle handle) = 0;

    virtual TextureHandle createTexture(const TextureDesc &desc) = 0;
    virtual void uploadToTexture(TextureHandle handle,
                                 std::span<const void *> data, uint32_t width,
                                 uint32_t height) = 0;
    inline void uploadToTexture(TextureHandle handle, const void *data,
                                uint32_t width, uint32_t height) {
        uploadToTexture(handle, std::span(&data, 1), width, height);
    }

    inline TextureHandle uploadTexture(const TextureDesc &desc,
                                       std::span<const void *> data) {
        TextureHandle handle{createTexture(desc)};
        uploadToTexture(handle, data, desc.width, desc.height);
        return handle;
    }

    inline TextureHandle uploadTexture(const TextureDesc &desc,
                                       const void *data) {
        TextureHandle handle{createTexture(desc)};
        uploadToTexture(handle, data, desc.width, desc.height);
        return handle;
    }

    virtual void destroyTexture(TextureHandle handle) = 0;

    virtual AttachmentHandle createAttachment(const AttachmentDesc &desc) = 0;
    virtual void destroyAttachment(AttachmentHandle &handle) = 0;

    virtual PipelineHandle createPipeline(const GraphicsPipelineDesc &desc) = 0;

    virtual void destroyPipeline(PipelineHandle &pipeline) = 0;

    virtual void beginFrame(Window &window) = 0;
    // submits and presents frame
    virtual void endFrame(Window &window) = 0;

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
    virtual void drawIndexedCmd(size_t nIndices, const BufferHandle& indexBuffer) = 0;
    virtual void drawImGUI() = 0;

    virtual AttachmentHandle getSwapchainAttachmentCmd() = 0;

    // returns the index of the texture in the bindless sampler2D array that
    // is valid to access in the renderPass
    virtual uint32_t getShaderSamplerIndexCmd(TextureHandle texture) = 0;

    // only for attachments that are sampleable
    virtual uint32_t getShaderSamplerIndexCmd(AttachmentHandle attachment) = 0;
    virtual uint64_t getBufferAddressCmd(BufferHandle buffer) = 0;

    virtual void shutdown() = 0;
};

} // namespace SYN
