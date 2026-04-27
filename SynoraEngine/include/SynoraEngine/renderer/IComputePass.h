#pragma once
#include "RenderTypes.h"

namespace SYN {

class GraphicsCommandBuffer;

class IComputePass {
  public:
    IComputePass() = default;
    virtual ~IComputePass() = default;
    virtual void execute(GraphicsCommandBuffer &cmdBuffer,
                         PipelineHandle pipeline) = 0;
    virtual DispatchDesc getPassDesc() = 0;
    virtual ComputePipelineDesc getPipelineDesc() const = 0;

    RenderPassNode getNode() {
        RenderPassNode node{};

        DispatchDesc desc{this->getPassDesc()};
        node.debugName = desc.debugName;

        for (const auto &attachment : desc.readonlyAttachments) {
            node.inputAttachments.emplace_back(attachment);
        }

        for (const auto &attachment : desc.readonlyAttachments) {
            node.inputAttachments.emplace_back(attachment);
            node.outputAttachments.emplace_back(attachment);
        }

        return node;
    }
};
} // namespace SYN
