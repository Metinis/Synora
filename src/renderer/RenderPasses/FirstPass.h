#pragma once
#include "renderer/IRenderPass.h"
#include "renderer/RenderTypes.h"

namespace SYN {
class FirstPass : public IRenderPass {
  public:
    FirstPass(BufferHandle vertexBuffer, TextureHandle textureHandle,
              AttachmentHandle colorAttachment,
              AttachmentHandle depthAttachment);

    void execute(IBackend &backend, PipelineHandle pipeline) override;
    GraphicsPipelineDesc getPipelineDesc() const override;
    RenderPassDesc getPassDesc() const override;

  private:
    BufferHandle m_VertexBuffer;
    TextureHandle m_TextureHandle;

    WriteAttachment m_ColorAttachment;
    WriteAttachment m_DepthAttachment;
};
} // namespace SYN
