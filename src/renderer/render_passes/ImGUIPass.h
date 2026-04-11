#pragma once
#include "imgui.h"
#include "renderer/IRenderPass.h"

namespace SYN {
    class ImGUIPass : public IRenderPass {
    public:
        ImGUIPass() = default;
        void execute(IBackend &backend, PipelineHandle pipeline) override;
        RenderPassDesc getPassDesc() const override;
        GraphicsPipelineDesc getPipelineDesc() const override;
    };
}
