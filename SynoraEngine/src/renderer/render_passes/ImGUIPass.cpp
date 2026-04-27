#include "ImGUIPass.h"

SYN::ImGUIPass::ImGUIPass(AttachmentHandle colorAttachment) {
    m_ColorAttachment = WriteAttachmentInfo{.handle = colorAttachment,
                                            .storeOp = StoreOp::store,
                                            .loadOp = LoadOp::load,
                                            .clearColor = {0.f, 0.f, 0.f, 1.f}};
}

void SYN::ImGUIPass::execute(IBackend &backend, PipelineHandle pipeline) {
    backend.beginRenderPassCmd(getPassDesc(), pipeline);

    backend.drawImGUI();

    backend.endRenderPassCmd();
}

SYN::RenderPassDesc SYN::ImGUIPass::getPassDesc() {
    return {
        .debugName = "ImGUI Pass",
        .colorAttachments = std::span(&m_ColorAttachment, 1),
    };
}

SYN::GraphicsPipelineDesc SYN::ImGUIPass::getPipelineDesc() const {
    constexpr GraphicsPipelineDesc c_PipelineDesc{
        .cullMode = CullMode::backFace,
        .nColorAttachments = 1,
        .vertexShaderPath = "generated/shaders/first.vert.spv",
        .fragmentShaderPath = "generated/shaders/first.frag.spv",
    };
    return c_PipelineDesc;
}
