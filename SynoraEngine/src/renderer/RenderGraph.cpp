#include "RenderGraph.h"
#include "../../include/SynoraEngine/renderer/RenderTypes.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <queue>
#include <unordered_map>

using namespace SYN;

namespace {
std::vector<size_t>
makeTopoSorted(const std::vector<std::unique_ptr<IRenderPass>> &renderPasses);
}

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

    std::vector<size_t> sortedPassIndices{makeTopoSorted(m_Passes)};

    for (const auto &passIndex : sortedPassIndices) {
        IRenderPass *pass{m_Passes[passIndex].get()};
        PipelineHandle pipeline{m_PipelineHandles[passIndex]};
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

namespace {

std::vector<size_t>
makeTopoSorted(const std::vector<std::unique_ptr<IRenderPass>> &renderPasses) {
    size_t passCount{renderPasses.size()};
    std::vector<size_t> passInDegree(passCount, 0);
    std::queue<size_t> passQueue{};
    std::vector<std::vector<size_t>> passAdjList(passCount);

    std::unordered_map<AttachmentHandle, size_t> attachmentToLastOutputPass{};
    for (size_t i{}; i < passCount; i++) {
        const RenderPassNode &node{renderPasses[i]->getNode()};

        for (auto attachment : node.inputAttachments) {
            if (!attachmentToLastOutputPass.contains(attachment)) {
                spdlog::warn("{} contains input attachment that was "
                             "not yet produced by a renderpass in submission "
                             "order to the render graph",
                             node.debugName);
                continue;
            }
            size_t predecessor{attachmentToLastOutputPass[attachment]};
            passAdjList[predecessor].emplace_back(i);
            passInDegree[i]++;
        }

        for (auto attachment : node.outputAttachments) {
            attachmentToLastOutputPass[attachment] = i;
        }
    }

    for (size_t i{}; i < passInDegree.size(); i++) {
        if (passInDegree[i] == 0) {
            passQueue.push(i);
        }
    }

    std::vector<size_t> sortedPasses{};
    sortedPasses.reserve(passCount);
    while (!passQueue.empty() && sortedPasses.size() < passCount) {
        size_t frontPass{passQueue.front()};
        passQueue.pop();
        sortedPasses.emplace_back(frontPass);

        for (auto descendantPass : passAdjList[frontPass]) {
            passInDegree[descendantPass]--;
            if (passInDegree[descendantPass] == 0) {
                passQueue.push(descendantPass);
            }
        }
    }

    if (sortedPasses.size() < passCount) {
        spdlog::warn("Render graph contains cycles");
    }
    return sortedPasses;
}
} // namespace
