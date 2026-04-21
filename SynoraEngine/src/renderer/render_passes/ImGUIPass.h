#pragma once
#include "../../../include/SynoraEngine/renderer/IRenderPass.h"
#include "imgui.h"

namespace SYN {
class ImGUIPass : public IRenderPass {
  public:
    ImGUIPass(AttachmentHandle colorAttachment);

    void execute(IBackend &backend, PipelineHandle pipeline) override;
    RenderPassDesc getPassDesc() override;
    GraphicsPipelineDesc getPipelineDesc() const override;

  private:
    WriteAttachmentInfo m_ColorAttachment;
};
} // namespace SYN
