#pragma once
#include "SynoraEngine/renderer/IComputePass.h"
#include "SynoraEngine/renderer/IRenderPass.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "renderer/backends/CommandBuffers.h"
#include "renderer/backends/RenderDevice.h"
#include <concepts>
#include <memory>
#include <type_traits>
#include <typeindex>
#include <typeinfo>

namespace SYN {

class RenderGraph {
  public:
    template <typename T, typename... Args> void addPass(Args &&...args) {
        if constexpr (std::is_base_of<IRenderPass, T>()) {
            m_RenderPasses.emplace_back(
                std::make_unique<T>(std::forward<Args>(args)...));
            m_PassNodes.emplace_back(m_RenderPasses.back()->getNode());
        } else if constexpr (std::is_base_of<IComputePass, T>()) {
            m_ComputePasses.emplace_back(
                std::make_unique<T>(std::forward<Args>(args)...));
            m_PassNodes.emplace_back(m_ComputePasses.back()->getNode());
        }
    }

    void compile(RenderDevice &renderDevice);
    void execute(GraphicsCommandBuffer &cmdBuffer);

    template <typename T>
    void executeImmediately(RenderDevice &renderDevice,
                            ComputeCommandBuffer &cmdBuffer, T computePass) {
        static_assert(std::is_base_of<IComputePass, T>());

        ComputePipelineDesc pipelineDesc{computePass.getPipelineDesc()};

        PipelineHandle pipelineHandle{};
        if (!m_ComputePipelineCache.contains(pipelineDesc)) {
            pipelineHandle = renderDevice.createPipeline(pipelineDesc);
            m_ComputePipelineCache[pipelineDesc] = pipelineHandle;
        } else {
            pipelineHandle = m_ComputePipelineCache[pipelineDesc];
        }

        computePass.execute(cmdBuffer, pipelineHandle);
    }

    void shutdown(RenderDevice &renderDevice);

  private:
    std::vector<std::unique_ptr<IRenderPass>> m_RenderPasses;
    std::vector<std::unique_ptr<IComputePass>> m_ComputePasses;
    std::vector<PassNode> m_PassNodes;

    std::vector<std::function<void(GraphicsCommandBuffer &)>> m_PassFunctors;

    std::unordered_map<GraphicsPipelineDesc, PipelineHandle>
        m_GraphicsPipelineCache;
    std::unordered_map<ComputePipelineDesc, PipelineHandle>
        m_ComputePipelineCache;
    bool m_Compiled{};
};
} // namespace SYN
