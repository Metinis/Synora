#include "ImGUIPass.h"

void SYN::ImGUIPass::execute(IBackend &backend, PipelineHandle pipeline) {
    backend.drawImGUI();
}

SYN::RenderPassDesc SYN::ImGUIPass::getPassDesc() const {
    return {};
}

SYN::GraphicsPipelineDesc SYN::ImGUIPass::getPipelineDesc() const {
    constexpr GraphicsPipelineDesc c_PipelineDesc{
        .cullMode = CullMode::backFace,
        .nColorAttachments = 1,
        .hasDepthAttachment = false,
        .vertexShaderPath = "generated/shaders/first.vert.spv",
        .fragmentShaderPath = "generated/shaders/first.frag.spv",
    };
    return c_PipelineDesc;
}
