#pragma once
#include "imgui.h"
#include "renderer/IRenderPass.h"

namespace SYN {
class ImGUIPass : public IRenderPass {
  public:
    ImGUIPass(AttachmentHandle colorAttachment);

    void execute(IGraphicsContext &ctx, PipelineHandle pipeline) override;
    RenderPassDesc getPassDesc() override;
    GraphicsPipelineDesc getPipelineDesc() const override;

  private:
    WriteAttachmentInfo m_ColorAttachment;
};
} // namespace SYN
