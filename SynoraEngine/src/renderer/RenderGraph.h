#pragma once
#include "IRenderPass.h"
#include "renderer/RenderTypes.h"
#include "renderer/backends/IDevice.h"
#include "renderer/backends/IGraphicsContext.h"
#include <memory>
#include <typeindex>
#include <typeinfo>

namespace SYN {
class RenderGraph {
  public:
    template <typename T, typename... Args> void addPass(Args &&...args) {
        m_Passes.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
    }

    void compile(IDevice &device);
    void execute(IGraphicsContext &ctx);

    void shutdown(IDevice &device);

  private:
    std::vector<std::unique_ptr<IRenderPass>> m_Passes;

    std::vector<PipelineHandle> m_PipelineHandles;
    std::unordered_map<GraphicsPipelineDesc, PipelineHandle> m_PipelineCache;
    bool m_Compiled{};
};
} // namespace SYN
