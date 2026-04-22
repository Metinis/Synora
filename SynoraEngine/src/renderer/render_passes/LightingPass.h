#pragma once
#include "renderer/IRenderPass.h"
#include "renderer/RenderTypes.h"
#include "renderer/Renderer.h"

namespace SYN {
class LightingPass : public IRenderPass {
  public:
    LightingPass(uint32_t msaaSampleCount,
                 std::span<Renderer::MeshDrawCall> drawCalls,
                 const glm::mat4 &cameraProjection, const glm::mat4 &cameraView,
                 AttachmentHandle msaaColorAttachment,
                 AttachmentHandle msaaDepthAttachment,
                 AttachmentHandle colorAttachment);

    void execute(IGraphicsContext &ctx, PipelineHandle pipeline) override;
    GraphicsPipelineDesc getPipelineDesc() const override;
    RenderPassDesc getPassDesc() override;

  private:
    uint32_t m_MSAASampleCount;
    std::span<Renderer::MeshDrawCall> m_DrawCalls;
    glm::mat4 m_CameraProjection;
    glm::mat4 m_CameraView;

    WriteAttachmentInfo m_ColorAttachment;
    WriteAttachmentInfo m_DepthAttachment;
};
} // namespace SYN
