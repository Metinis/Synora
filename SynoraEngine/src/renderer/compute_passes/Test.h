#pragma once

#include "SynoraEngine/renderer/IComputePass.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "renderer/backends/vulkan/command_buffers/ComputeCommandBuffer.h"

namespace SYN {
class TestPass : public IComputePass {
  public:
    TestPass() = default;
    TestPass(AttachmentHandle colorImage, glm::vec2 imageResolution);

    void execute(GraphicsCommandBuffer &cmdBuffer,
                 PipelineHandle pipeline) override;
    void execute(ComputeCommandBuffer &cmdBuffer,
                 PipelineHandle pipeline) override;

    DispatchDesc getPassDesc() override;
    ComputePipelineDesc getPipelineDesc() const override;

  private:
    AttachmentHandle m_ColorImage;
    glm::vec2 m_ImageResolution;
};
} // namespace SYN
