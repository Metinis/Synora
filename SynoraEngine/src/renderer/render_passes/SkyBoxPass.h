#pragma once
#include "../../../include/SynoraEngine/renderer/IRenderPass.h"
#include "../../../include/SynoraEngine/renderer/RenderTypes.h"
#include "../../../include/SynoraEngine/renderer/Renderer.h"

namespace SYN {
class SkyBoxPass : public IRenderPass {
  public:
    SkyBoxPass(uint32_t msaaSampleCount, const glm::mat4 &cameraProjection,
               const glm::mat4 &cameraView, TextureHandle skyBox,
               AttachmentHandle msaaColorAttachment,
               AttachmentHandle msaaDepthAttachment,
               AttachmentHandle colorAttachment);

    void execute(IGraphicsContext &ctx, PipelineHandle pipeline) override;
    GraphicsPipelineDesc getPipelineDesc() const override;
    RenderPassDesc getPassDesc() override;

  private:
    uint32_t m_MSAASampleCount;

    glm::mat4 m_CameraProjection;
    glm::mat4 m_CameraView;

    TextureHandle m_SkyBox;

    WriteAttachmentInfo m_ColorAttachment;
    WriteAttachmentInfo m_DepthAttachment;
};
} // namespace SYN
