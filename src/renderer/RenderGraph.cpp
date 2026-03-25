#include "RenderGraph.h"
#include "renderer/RenderTypes.h"
#include "spdlog/spdlog.h"

using namespace SYN;

void RenderGraph::compile(IBackend &backend) {
    m_PipelineHandles.resize(m_Passes.size());
    for (size_t i{}; i < m_Passes.size(); i++) {
        IRenderPass *pass{m_Passes[i].get()};
        GraphicsPipelineDesc pipelineDesc{pass->getPipelineDesc()};
        if (!m_PipelineCache.contains(pipelineDesc)) {
            PipelineHandle pipeline{backend.createPipeline(pipelineDesc)};
            m_PipelineCache[pipelineDesc] = pipeline;
            m_PipelineHandles[i] = pipeline;
        } else {
            m_PipelineHandles[i] = m_PipelineCache[pipelineDesc];
        }
    }

    m_Compiled = true;
}

void RenderGraph::execute(IBackend &backend) {
    if (m_Compiled == false) {
        spdlog::error("Render graph not compiled before executing, could not "
                      "render frame");
        return;
    }

    for (const auto &pass : m_Passes) {
        RenderPassNode node{pass->getNode()};
    }
    for (size_t i{}; i < m_Passes.size(); i++) {
        IRenderPass *pass{m_Passes[i].get()};
        PipelineHandle pipeline{m_PipelineHandles[i]};
        pass->execute(backend, pipeline);
    }
    m_Passes.clear();
    m_PipelineHandles.clear();
    m_Compiled = false;
}

void RenderGraph::shutdown(IBackend &backend) {
    for (auto &[desc, pipeline] : m_PipelineCache) {
        backend.destroyPipeline(pipeline);
    }
}
