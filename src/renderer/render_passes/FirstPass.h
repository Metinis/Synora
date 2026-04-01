#pragma once
#include "renderer/IRenderPass.h"
#include "renderer/RenderTypes.h"
#include "renderer/Renderer.h"

namespace SYN {
class FirstPass : public IRenderPass {
  public:
    FirstPass(std::span<Renderer::MeshDrawCall> drawCalls,
              AttachmentHandle colorAttachment,
              AttachmentHandle depthAttachment);

    void execute(IBackend &backend, PipelineHandle pipeline) override;
    GraphicsPipelineDesc getPipelineDesc() const override;
    RenderPassDesc getPassDesc() const override;

  private:
    std::span<Renderer::MeshDrawCall> m_DrawCalls;

    WriteAttachment m_ColorAttachment;
    WriteAttachment m_DepthAttachment;
};
} // namespace SYN
