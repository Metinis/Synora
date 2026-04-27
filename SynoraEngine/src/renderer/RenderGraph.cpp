#include "RenderGraph.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "spdlog/spdlog.h"
#include <algorithm>
#include <queue>
#include <set>
#include <unordered_map>

#include "renderer/backends/RenderDevice.h"

using namespace SYN;

namespace {
std::vector<size_t> makeTopoSorted(std::span<PassNode> passNodes);
}

void RenderGraph::compile(RenderDevice &renderDevice) {
    m_PassFunctors.reserve(m_RenderPasses.size() + m_ComputePasses.size());

    for (auto &pass : m_RenderPasses) {
        GraphicsPipelineDesc pipelineDesc{pass->getPipelineDesc()};

        PipelineHandle pipelineHandle{};
        if (!m_GraphicsPipelineCache.contains(pipelineDesc)) {
            pipelineHandle = renderDevice.createPipeline(pipelineDesc);
            m_GraphicsPipelineCache[pipelineDesc] = pipelineHandle;
        } else {
            pipelineHandle = m_GraphicsPipelineCache[pipelineDesc];
        }

        m_PassFunctors.emplace_back(
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
        m_PassFunctors.emplace_back(
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

    std::vector<size_t> sortedPassIndices{makeTopoSorted(m_PassNodes)};

    for (const auto &passIndex : sortedPassIndices) {
        m_PassFunctors[passIndex](cmdBuffer);
    }

    m_RenderPasses.clear();
    m_ComputePasses.clear();
    m_PassFunctors.clear();
    m_PassNodes.clear();

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

std::vector<size_t> makeTopoSorted(std::span<PassNode> passNodes) {

    size_t passCount{passNodes.size()};
    std::vector<size_t> passInDegree(passCount, 0);
    std::queue<size_t> passQueue{};
    std::vector<std::unordered_set<size_t>> passAdjList(passCount);

    std::unordered_map<AttachmentHandle, size_t> attachmentToLastOutputPass{};

    for (size_t i{}; i < passCount; i++) {
        const PassNode &node{passNodes[i]};

        for (auto attachment : node.inputAttachments) {
            if (!attachmentToLastOutputPass.contains(attachment)) {
                spdlog::warn("{} contains input attachment that was "
                             "not yet produced by a renderpass in submission "
                             "order to the render graph",
                             node.debugName);
                continue;
            }
            size_t predecessor{attachmentToLastOutputPass[attachment]};
            if (passAdjList[predecessor].contains(i)) {
                continue;
            }

            passAdjList[predecessor].emplace(i);
            passInDegree[i]++;
        }

        if (passInDegree[i] == 0) {
            passQueue.push(i);
        }

        for (auto attachment : node.outputAttachments) {
            attachmentToLastOutputPass[attachment] = i;
        }
    }

    std::vector<size_t> sortedPasses{};
    sortedPasses.reserve(passCount);
    while (!passQueue.empty() && sortedPasses.size() < passCount) {
        size_t frontPass{passQueue.front()};
        passQueue.pop();
        sortedPasses.emplace_back(frontPass);

        for (auto neighbor : passAdjList[frontPass]) {
            passInDegree[neighbor]--;
            if (passInDegree[neighbor] == 0) {
                passQueue.push(neighbor);
            }
        }
    }

    if (sortedPasses.size() < passCount) {
        spdlog::error("Render graph contains cycles");
        assert(false);
    }
    return sortedPasses;
}
} // namespace
