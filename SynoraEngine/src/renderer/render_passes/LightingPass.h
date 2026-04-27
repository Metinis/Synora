#pragma once
#include "SynoraEngine/renderer/IRenderPass.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "SynoraEngine/renderer/Renderer.h"

namespace SYN {
class LightingPass : public IRenderPass {
  public:
    LightingPass(uint32_t msaaSampleCount,
                 std::span<Renderer::MeshDrawCall> drawCalls,
                 const glm::mat4 &cameraProjection, const glm::mat4 &cameraView,
                 AttachmentHandle msaaColorAttachment,
                 AttachmentHandle msaaDepthAttachment,
                 AttachmentHandle colorAttachment,
                 AttachmentHandle testAttachment);

    void execute(GraphicsCommandBuffer &cmdBuffer,
                 PipelineHandle pipeline) override;
    GraphicsPipelineDesc getPipelineDesc() const override;
    RenderPassDesc getPassDesc() override;

  private:
    uint32_t m_MSAASampleCount;
    std::span<Renderer::MeshDrawCall> m_DrawCalls;
    glm::mat4 m_CameraProjection;
    glm::mat4 m_CameraView;

    WriteAttachmentInfo m_ColorAttachment;
    WriteAttachmentInfo m_DepthAttachment;
    AttachmentHandle m_TestAttachment;
};
} // namespace SYN
