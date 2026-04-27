#include "SkyBoxPass.h"
#include "../../../include/SynoraEngine/renderer/RenderTypes.h"
#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <spdlog/spdlog.h>

using namespace SYN;

namespace {
struct alignas(16) PushConstants {
    uint32_t cubeMapIndex;
};

struct Uniforms {
    glm::mat4 invProjectionViewMat;
};
} // namespace

SYN::SkyBoxPass::SkyBoxPass(uint32_t msaaSampleCount,
                            const glm::mat4 &cameraProjection,
                            const glm::mat4 &cameraView, TextureHandle skyBox,
                            AttachmentHandle msaaColorAttachment,
                            AttachmentHandle msaaDepthAttachment,
                            AttachmentHandle colorAttachment)
    : m_MSAASampleCount(msaaSampleCount), m_SkyBox(skyBox),
      m_CameraProjection(cameraProjection), m_CameraView(cameraView) {

    m_ColorAttachment = WriteAttachmentInfo{.handle = msaaColorAttachment,
                                            .resolveHandle = colorAttachment,
                                            .storeOp = StoreOp::store,
                                            .loadOp = LoadOp::load};
    m_DepthAttachment = WriteAttachmentInfo{.handle = msaaDepthAttachment,
                                            .storeOp = StoreOp::dontCare,
                                            .loadOp = LoadOp::load};
}

void SYN::SkyBoxPass::execute(IBackend &backend, PipelineHandle pipeline) {
    Uniforms uniform{.invProjectionViewMat =
                         glm::inverse(m_CameraProjection *
                                      glm::mat4(glm::mat3(m_CameraView)))};

    backend.beginRenderPassCmd(getPassDesc(), pipeline, uniform);

    PushConstants pushConstants{
        .cubeMapIndex = backend.getShaderSamplerIndexCmd(m_SkyBox),
    };

    backend.setPushConstantsCmd(pushConstants);

    backend.drawCmd(6); // 6 vertices in 2 triangles for full screen pass
    backend.endRenderPassCmd();
}

RenderPassDesc SYN::SkyBoxPass::getPassDesc() {
    return RenderPassDesc{
        .debugName = "SkyBox Pass",
        .colorAttachments = std::span(&m_ColorAttachment, 1),
        .depthAttachment = m_DepthAttachment,
    };
}

GraphicsPipelineDesc SYN::SkyBoxPass::getPipelineDesc() const {
    GraphicsPipelineDesc c_PipelineDesc{
        .cullMode = CullMode::disabled,
        .nColorAttachments = 1,
        .hasDepthTesting = true,
        .hasDepthWriting = false,
        .msaaSamples = m_MSAASampleCount,
        .vertexShaderPath = "generated/shaders/skybox.vert.spv",
        .fragmentShaderPath = "generated/shaders/skybox.frag.spv",
    };
    return c_PipelineDesc;
}
