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

struct LightCaster {
    glm::vec3 pos;
    float padding0;
    glm::vec3 color;
    float padding1;
    glm::vec3 dir;
    float padding2;
};

struct Uniforms {
    glm::mat4 projectionMat;
    glm::mat4 viewMat;
    std::array<LightCaster, 16> lights;
    uint32_t nLights;
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
    LightCaster sun{.pos = glm::vec3(0.f, 20.f, 0.f),
                    .color = glm::vec3(1.f, 1.f, 1.f),
                    .dir = glm::vec3(1.f, -1.f, 0.f)};

    Uniforms uniform{
        .projectionMat = m_CameraProjection,
        .viewMat = m_CameraView,
        .nLights = 1,
    };
    uniform.lights[0] = sun;

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
        .debugName = "First Pass",
        .colorAttachments = std::span(&m_ColorAttachment, 1),
        .depthAttachment = m_DepthAttachment,
    };
}

GraphicsPipelineDesc SYN::FirstPass::getPipelineDesc() const {
    GraphicsPipelineDesc c_PipelineDesc{
        .cullMode = CullMode::disabled,
        .nColorAttachments = 1,
        .hasDepthTesting = true,
        .hasDepthWriting = true,
        .msaaSamples = m_MSAASampleCount,
        .vertexShaderPath = "generated/shaders/first.vert.spv",
        .fragmentShaderPath = "generated/shaders/first.frag.spv",
    };
    return c_PipelineDesc;
}
