#include "Test.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "renderer/backends/RenderDevice.h"

struct PushConstants {
    uint32_t imageIndex0;
    uint32_t imageIndex1;
    glm::uvec2 resolution;
};

SYN::TestPass::TestPass(AttachmentHandle colorImage, glm::vec2 imageResolution)
    : m_ColorImage(colorImage), m_ImageResolution(imageResolution) {}

void SYN::TestPass::execute(GraphicsCommandBuffer &cmdBuffer,
                            PipelineHandle pipeline) {

    PushConstants pushConstants{
        .imageIndex0 = cmdBuffer.getShaderStorageImageIndexCmd(m_ColorImage, 0),
        .imageIndex1 = cmdBuffer.getShaderStorageImageIndexCmd(m_ColorImage, 1),
        .resolution = m_ImageResolution,
    };

    cmdBuffer.setPushConstantsCmd(pushConstants);
    cmdBuffer.dispatchCmd(getPassDesc(), pipeline);
}

SYN::DispatchDesc SYN::TestPass::getPassDesc() {
    return {
        .debugName = "Test Pass",
        .writeonlyAttachments = std::span(&m_ColorImage, 1),
        .groupCountX = static_cast<uint32_t>(m_ImageResolution.x / 16),
        .groupCountY = static_cast<uint32_t>(m_ImageResolution.y / 16),
        .groupCountZ = 1,
    };
}

SYN::ComputePipelineDesc SYN::TestPass::getPipelineDesc() const {
    constexpr ComputePipelineDesc c_PipelineDesc{
        .shaderPath = "generated/shaders/test.comp.spv"};
    return c_PipelineDesc;
}
