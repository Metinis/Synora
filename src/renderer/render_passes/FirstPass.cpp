#include "FirstPass.h"
#include "renderer/RenderTypes.h"
#include <glm/ext.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <spdlog/spdlog.h>

using namespace SYN;

namespace {
struct alignas(16) PushConstants {
    glm::mat4 modelMat;

    uint64_t vertexBuffer;
    uint64_t indexBuffer;
    uint32_t textureIndex;
};

struct Uniforms {
    glm::mat4 projectionViewMat;
};
} // namespace

SYN::FirstPass::FirstPass(uint32_t msaaSampleCount,
                          std::span<Renderer::MeshDrawCall> drawCalls,
                          const glm::mat4 &cameraProjection,
                          const glm::mat4 &cameraView,
                          AttachmentHandle msaaColorAttachment,
                          AttachmentHandle msaaDepthAttachment,
                          AttachmentHandle colorAttachment)
    : m_MSAASampleCount(msaaSampleCount), m_DrawCalls(drawCalls),
      m_CameraProjection(cameraProjection), m_CameraView(cameraView) {
    m_ColorAttachment = WriteAttachmentInfo{.handle = msaaColorAttachment,
                                            .resolveHandle = colorAttachment,
                                            .clearColor = {0.f, 0.f, 0.f, 1.f}};
    m_DepthAttachment = WriteAttachmentInfo{
        .handle = msaaDepthAttachment,
        .clearDepth = 1.f,
    };
}

void SYN::FirstPass::execute(IBackend &backend, PipelineHandle pipeline) {
    Uniforms uniform{.projectionViewMat = m_CameraProjection * m_CameraView};

    backend.beginRenderPassCmd(getPassDesc(), pipeline, uniform);

    for (const auto &drawCall : m_DrawCalls) {
        if (drawCall.mesh.numIndices == 0) {
            continue;
        }

        PushConstants pushConstants{
            .modelMat = drawCall.modelMatrix,
            .vertexBuffer =
                backend.getBufferAddressCmd(drawCall.mesh.vertexBuffer),
            .indexBuffer =
                backend.getBufferAddressCmd(drawCall.mesh.indexBuffer),
            .textureIndex =
                backend.getShaderSamplerIndexCmd(drawCall.mesh.albedo),
        };

        backend.setPushConstantsCmd(pushConstants);

        backend.drawCmd(drawCall.mesh.numIndices);
    }
    backend.endRenderPassCmd();
}

RenderPassDesc SYN::FirstPass::getPassDesc() {
    return RenderPassDesc{
        .colorAttachments = std::span(&m_ColorAttachment, 1),
        .depthAttachment = m_DepthAttachment,
    };
}

GraphicsPipelineDesc SYN::FirstPass::getPipelineDesc() const {
    GraphicsPipelineDesc c_PipelineDesc{
        .cullMode = CullMode::backFace,
        .nColorAttachments = 1,
        .hasDepthAttachment = true,
        .msaaSamples = m_MSAASampleCount,
        .vertexShaderPath = "generated/shaders/first.vert.spv",
        .fragmentShaderPath = "generated/shaders/first.frag.spv",
    };
    return c_PipelineDesc;
}
