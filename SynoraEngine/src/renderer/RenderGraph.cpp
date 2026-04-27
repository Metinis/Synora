#include "RenderGraph.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <queue>
#include <unordered_map>

#include "renderer/backends/RenderDevice.h"

using namespace SYN;

namespace {
std::vector<size_t> makeTopoSorted(std::span<RenderPassNode> passNodes);
}

void RenderGraph::compile(RenderDevice &renderDevice) {
    m_Passes.reserve(m_RenderPasses.size() + m_ComputePasses.size());

    for (auto &pass : m_RenderPasses) {
        GraphicsPipelineDesc pipelineDesc{pass->getPipelineDesc()};

        PipelineHandle pipelineHandle{};
        if (!m_GraphicsPipelineCache.contains(pipelineDesc)) {
            pipelineHandle = renderDevice.createPipeline(pipelineDesc);
            m_GraphicsPipelineCache[pipelineDesc] = pipelineHandle;
        } else {
            pipelineHandle = m_GraphicsPipelineCache[pipelineDesc];
        }

        m_Passes.emplace_back(
            [pipelineHandle, &pass](GraphicsCommandBuffer &cmdBuffer) {
                pass->execute(cmdBuffer, pipelineHandle);
            });
    }

    for (auto &pass : m_ComputePasses) {
        ComputePipelineDesc pipelineDesc{pass->getPipelineDesc()};

        PipelineHandle pipelineHandle{};
        if (!m_ComputePipelineCache.contains(pipelineDesc)) {
            pipelineHandle = renderDevice.createPipeline(pipelineDesc);
            m_ComputePipelineCache[pipelineDesc] = pipelineHandle;
        } else {
            pipelineHandle = m_ComputePipelineCache[pipelineDesc];
        }
        m_Passes.emplace_back(
            [pipelineHandle, &pass](GraphicsCommandBuffer &cmdBuffer) {
                pass->execute(cmdBuffer, pipelineHandle);
            });
    }

    m_Compiled = true;
}

void RenderGraph::execute(GraphicsCommandBuffer &cmdBuffer) {
    if (m_Compiled == false) {
        spdlog::error("Render graph not compiled before executing, could not "
                      "render frame");
        return;
    }

    // std::vector<size_t> sortedPassIndices{makeTopoSorted(m_RenderPassNodes)};
    std::vector<size_t> sortedPassIndices{};
    for (size_t i{}; i < m_RenderPassNodes.size(); i++) {
        sortedPassIndices.emplace_back(i);
    }

    for (const auto &passIndex : sortedPassIndices) {
        m_Passes[passIndex](cmdBuffer);
    }

    m_RenderPasses.clear();
    m_ComputePasses.clear();
    m_Passes.clear();
    m_RenderPassNodes.clear();

    m_Compiled = false;
}

void RenderGraph::shutdown(RenderDevice &renderDevice) {
    for (auto &[desc, pipeline] : m_GraphicsPipelineCache) {
        renderDevice.destroyPipeline(pipeline);
    }
    for (auto &[desc, pipeline] : m_ComputePipelineCache) {
        renderDevice.destroyPipeline(pipeline);
    }
}

namespace {

std::vector<size_t> makeTopoSorted(std::span<RenderPassNode> passNodes) {

    size_t passCount{passNodes.size()};
    std::vector<size_t> passInDegree(passCount, 0);
    std::queue<size_t> passQueue{};
    std::vector<std::vector<size_t>> passAdjList(passCount);

    std::unordered_map<AttachmentHandle, size_t> attachmentToLastOutputPass{};
    for (size_t i{}; i < passCount; i++) {
        const RenderPassNode &node{passNodes[i]};

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
