#include "FirstPass.h"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include "renderer/RenderTypes.h"

using namespace SYN;

namespace {
struct PushConstants {
    uint64_t vertexBuffer;
    uint32_t textureIndex;
};

struct Uniforms {
    glm::mat4 modelMat;
};
} // namespace

SYN::FirstPass::FirstPass(BufferHandle vertexBuffer,
                          TextureHandle textureHandle,
                          AttachmentHandle colorAttachment,
                          AttachmentHandle depthAttachment)
    : m_VertexBuffer(vertexBuffer), m_TextureHandle(textureHandle) {
    m_ColorAttachment = WriteAttachment{.handle = colorAttachment};
    m_DepthAttachment = WriteAttachment{.handle = depthAttachment};
}

void SYN::FirstPass::execute(IBackend &backend, PipelineHandle pipeline) {
    PushConstants pushConstants{
        .vertexBuffer = backend.getBufferAddressCmd(m_VertexBuffer),
        .textureIndex = backend.getShaderSamplerIndexCmd(m_TextureHandle),
    };

    glm::mat4 modelMat(1.f);
    modelMat = glm::translate(modelMat, glm::vec3(1.f, 0.f, 0.f));
    Uniforms uniform{.modelMat = modelMat};

    backend.beginRenderPassCmd(getPassDesc(), pipeline, uniform);
    backend.setPushConstantsCmd(pushConstants);

    backend.drawCmd(6);
    backend.endRenderPassCmd();
}

RenderPassDesc SYN::FirstPass::getPassDesc() const {
    return RenderPassDesc{
        .colorAttachments = std::span(&m_ColorAttachment, 1),
        .depthAttachment = m_DepthAttachment,
    };
}

GraphicsPipelineDesc SYN::FirstPass::getPipelineDesc() const {
    constexpr GraphicsPipelineDesc c_PipelineDesc{
        .cullMode = CullMode::backFace,
        .nColorAttachments = 1,
        .hasDepthAttachment = true,
        .vertexShaderPath = "generated/shaders/first.vert.spv",
        .fragmentShaderPath = "generated/shaders/first.frag.spv",
    };
    return c_PipelineDesc;
}
