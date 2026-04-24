#include "LightingPass.h"
#include "SynoraEngine/renderer/RenderTypes.h"
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

    uint32_t albedoIndex;
    uint32_t metallicRoughnessIndex;
    uint32_t normalMapIndex;
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
    glm::vec3 cameraPos;
    uint32_t nLights;
    std::array<LightCaster, 16> lights;
};
} // namespace

SYN::LightingPass::LightingPass(uint32_t msaaSampleCount,
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

void SYN::LightingPass::execute(GraphicsCommandBuffer &cmdBuffer,
                                PipelineHandle pipeline) {
    LightCaster sun{.pos = glm::vec3(0.f, 1.f, -1.f),
                    .color = glm::vec3(1.f, 1.f, 1.f),
                    .dir = glm::vec3(1.f, -1.f, 0.f)};
    sun.color *= 10.f;
    LightCaster lamp{.pos = glm::vec3(1.f, 1.f, -1.f),
                     .color = glm::vec3(1.f, 0.f, 1.f),
                     .dir = glm::vec3(1.f, -1.f, 0.f)};
    lamp.color *= 5.f;

    std::array<LightCaster, 16> lights{sun, lamp};

    glm::vec3 cameraPos{glm::transpose(glm::mat3(m_CameraView)) *
                        -m_CameraView[3]};

    Uniforms uniform{
        .projectionMat = m_CameraProjection,
        .viewMat = m_CameraView,
        .cameraPos = cameraPos,
        .nLights = 2,
    };
    uniform.lights = lights;

    cmdBuffer.beginRenderPassCmd(getPassDesc(), pipeline, uniform);

    for (const auto &drawCall : m_DrawCalls) {
        if (drawCall.mesh.numIndices == 0) {
            continue;
        }

        PushConstants pushConstants{
            .modelMat = drawCall.modelMatrix,
            .vertexBuffer =
                cmdBuffer.getBufferAddressCmd(drawCall.mesh.vertexBuffer),
            .indexBuffer =
                cmdBuffer.getBufferAddressCmd(drawCall.mesh.indexBuffer),
            .albedoIndex =
                cmdBuffer.getShaderTextureIndexCmd(drawCall.mesh.albedo),
            .metallicRoughnessIndex = cmdBuffer.getShaderTextureIndexCmd(
                drawCall.mesh.metallicRoughness),
            .normalMapIndex =
                cmdBuffer.getShaderTextureIndexCmd(drawCall.mesh.normalMap),
        };

        cmdBuffer.setPushConstantsCmd(pushConstants);

        cmdBuffer.drawCmd(drawCall.mesh.numIndices);
    }
    cmdBuffer.endRenderPassCmd();
}

RenderPassDesc SYN::LightingPass::getPassDesc() {
    return RenderPassDesc{
        .debugName = "First Pass",
        .colorAttachments = std::span(&m_ColorAttachment, 1),
        .depthAttachment = m_DepthAttachment,
    };
}

GraphicsPipelineDesc SYN::LightingPass::getPipelineDesc() const {
    GraphicsPipelineDesc c_PipelineDesc{
        .cullMode = CullMode::disabled,
        .nColorAttachments = 1,
        .hasDepthTesting = true,
        .hasDepthWriting = true,
        .msaaSamples = m_MSAASampleCount,
        .vertexShaderPath = "generated/shaders/lighting.vert.spv",
        .fragmentShaderPath = "generated/shaders/lighting.frag.spv",
    };
    return c_PipelineDesc;
}
