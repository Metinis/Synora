#pragma once
#include "SynoraEngine/renderer/IRenderPass.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "renderer/backends/CommandBuffers.h"
#include "renderer/backends/RenderDevice.h"
#include "renderer/backends/vulkan/GraphicsCommandBuffer.h"
#include <memory>
#include <typeindex>
#include <typeinfo>

namespace SYN {
class RenderGraph {
  public:
    template <typename T, typename... Args> void addPass(Args &&...args) {
        m_Passes.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void compile(RenderDevice &renderDevice);
    void execute(GraphicsCommandBuffer &cmdBuffer);

    void shutdown(RenderDevice &renderDevice);

  private:
    std::vector<std::unique_ptr<IRenderPass>> m_Passes;

    std::vector<PipelineHandle> m_PipelineHandles;
    std::unordered_map<GraphicsPipelineDesc, PipelineHandle> m_PipelineCache;
    bool m_Compiled{};
};
} // namespace SYN
