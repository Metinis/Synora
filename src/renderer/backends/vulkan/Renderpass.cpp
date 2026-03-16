#include "Renderpass.h"
#include "Image.h"
#include <vulkan/vulkan_core.h>

using namespace SYN::VK;
using namespace SYN;

void SYN::VK::cmdDefaultRenderPass(VkCommandBuffer cmdBuffer,
                                   VkPipeline pipeline, Image &renderTarget) {

    constexpr VkClearValue colorClearValue{
        .color = VkClearColorValue{{0.f, 0.f, 0.f, 0.f}}};

    VkRenderingAttachmentInfo colorAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = renderTarget.view,
        .imageLayout = renderTarget.currentLayout,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = colorClearValue,
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = VkRect2D{.extent = renderTarget.extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,

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

    vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
    vkCmdEndRendering(cmdBuffer);
}
