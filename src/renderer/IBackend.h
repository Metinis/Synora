#pragma once
#include "PuzzleEngine/core/Application.h"
#include "PuzzleEngine/project/Assets.h"
#include "PuzzleEngine/project/UUID.h"
#include "RenderTypes.h"

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
                                void *data) = 0;
    virtual void destroyBuffer(BufferHandle &handle) = 0;

    virtual TextureHandle createTexture(const TextureDesc &desc) = 0;
    virtual void uploadToTexture(TextureHandle handle, uint32_t width,
                                 uint32_t height, uint32_t stride,
                                 void *data) = 0;
    virtual void destroyTexture(TextureHandle &handle) = 0;

    virtual AttachmentHandle createAttachment(const AttachmentDesc &desc) = 0;
    virtual void destroyAttachment(AttachmentHandle &handle) = 0;

    virtual void beginFrame(Window &window) = 0;
    // submits and presents frame
    virtual void endFrame(Window &window) = 0;

    virtual void beginRenderPassCmd(const RenderPassDesc &desc) = 0;
    virtual void endRenderPassCmd() = 0;

    virtual void setPushConstantsCmd(const void *data, size_t size) = 0;
    template <typename T> void setPushConstantsCmd(const T &pushConstants) {
        setPushConstantsCmd(&pushConstants, sizeof(pushConstants));
    }

    // virtual void uploadUniformsCmd(RenderPassHandle renderPass,
    //                                const void *uniformData,
    //                                size_t uniformSize) = 0;

    // template <typename T>
    // void uploadUniformsCmd(RenderPassHandle renderPass, const T &uniformData)
    // {
    //     uploadUniformsCmd(renderPass, &uniformData, sizeof(uniformData));
    // }

    virtual void drawCmd(BufferHandle vertexBuffer, size_t nVertices) = 0;

    virtual AttachmentHandle getSwapchainAttachmentCmd() = 0;
    virtual Viewport getSwapchainViewport() = 0;

    // returns the index of the texture in the bindless sampler2D array that
    // is valid to access in the renderPass
    virtual uint32_t getShaderSamplerIndexCmd(TextureHandle texture) = 0;

    // only for attachments that are sampleable
    virtual uint32_t getShaderSamplerIndexCmd(AttachmentHandle attachment) = 0;
    virtual uint64_t getBufferAddressCmd(BufferHandle buffer) = 0;

    virtual void shutdown() = 0;

  private:
};

} // namespace SYN
