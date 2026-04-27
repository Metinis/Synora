#pragma once
#include "RenderTypes.h"

namespace SYN {

class GraphicsCommandBuffer;

class IRenderPass {
  public:
    IRenderPass() = default;
    virtual ~IRenderPass() = default;
    virtual void execute(GraphicsCommandBuffer &cmdBuffer,
                         PipelineHandle pipeline) = 0;
    virtual RenderPassDesc getPassDesc() = 0;
    virtual GraphicsPipelineDesc getPipelineDesc() const = 0;

    RenderPassNode getNode() {
        RenderPassNode node{};

        RenderPassDesc desc{this->getPassDesc()};
        node.debugName = desc.debugName;
        if (desc.depthAttachment.has_value()) {
            if (desc.depthAttachment->loadOp == LoadOp::load) {
                node.inputAttachments.emplace_back(
                    desc.depthAttachment->handle);
            }
            node.outputAttachments.emplace_back(desc.depthAttachment->handle);

            if (desc.depthAttachment.value().resolveHandle) {
                node.outputAttachments.emplace_back(
                    desc.depthAttachment.value().resolveHandle.value());
            }
        }

        for (const auto &attachment : desc.colorAttachments) {
            if (attachment.loadOp == LoadOp::load) {
                node.inputAttachments.emplace_back(attachment.handle);
            }
            node.outputAttachments.emplace_back(attachment.handle);

            if (attachment.resolveHandle) {
                node.outputAttachments.emplace_back(
                    attachment.resolveHandle.value());
            }
        }

        for (const auto &handle : desc.readAttachments) {
            node.inputAttachments.emplace_back(handle);
        }

        return node;
    }
};
} // namespace SYN
