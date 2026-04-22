#pragma once
#include "IGraphicsContext.h"
#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/project/Assets.h"
#include "SynoraEngine/project/UUID.h"

struct MeshData;

namespace SYN {
class Window;

class IDevice {
  public:
    IDevice() = default;
    virtual ~IDevice() = default;

    virtual void init(Window *window) = 0;

    virtual BufferHandle createBuffer(const BufferDesc &desc) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;

    virtual TextureHandle createTexture(const TextureDesc &desc) = 0;
    virtual void destroyTexture(TextureHandle handle) = 0;

    virtual AttachmentHandle createAttachment(const AttachmentDesc &desc) = 0;
    virtual void destroyAttachment(AttachmentHandle &attachment) = 0;

    virtual PipelineHandle createPipeline(const GraphicsPipelineDesc &desc) = 0;
    virtual void destroyPipeline(PipelineHandle &pipeline) = 0;

    virtual std::unique_ptr<IGraphicsContext> makeGraphicsContext() = 0;

    virtual void shutdown() = 0;
};

} // namespace SYN
