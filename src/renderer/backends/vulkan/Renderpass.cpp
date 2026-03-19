#include "Renderpass.h"
#include "Image.h"
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

void SYN::VK::defaultRenderPassCmd(
    VkCommandBuffer cmdBuffer, VkPipeline pipeline, VkPipelineLayout layout,
    const PushConstants &pushConstants, VkDescriptorSet bindlessDescriptorSet,
    uint32_t vertexCount, const Image &renderTarget, const Image &depthTarget,
    VkImageLayout renderTargetLayout) {

    vkCmdBindDescriptorSets(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout,
                            0, 1, &bindlessDescriptorSet, 0, nullptr);

    constexpr VkClearValue colorClearValue{
        .color = VkClearColorValue{{0.f, 0.f, 0.f, 0.f}}};

    constexpr VkClearValue depthClearValue{.depthStencil =
                                               VkClearDepthStencilValue{0.f}};

    VkRenderingAttachmentInfo colorAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = renderTarget.view,
        .imageLayout = renderTargetLayout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = colorClearValue,
    };

    VkRenderingAttachmentInfo depthAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = depthTarget.view,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = depthClearValue,
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = VkRect2D{.extent = renderTarget.extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,
        .pDepthAttachment = &depthAttachmentInfo,
    };
    vkCmdBeginRendering(cmdBuffer, &renderingInfo);
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    VkViewport viewport{.y = static_cast<float>(renderTarget.extent.height),
                        .width = static_cast<float>(renderTarget.extent.width),
                        .height =
                            -static_cast<float>(renderTarget.extent.height),
                        .minDepth = 0.f,
                        .maxDepth = 1.f};

    VkRect2D scissor{.extent = renderTarget.extent};

    vkCmdSetViewport(cmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(cmdBuffer, 0, 1, &scissor);

    vkCmdPushConstants(cmdBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(PushConstants), &pushConstants);

    vkCmdDraw(cmdBuffer, vertexCount, 1, 0, 0);
    vkCmdEndRendering(cmdBuffer);
}
