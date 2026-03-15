#pragma once

#include <span>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {
struct Device;

class GraphicsPipeline {
  public:
    GraphicsPipeline();
    ~GraphicsPipeline() = default;

    void reset();
    VkPipeline build(const Device &device);

    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCIs;
    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachmentStates;

    VkPipelineVertexInputStateCreateInfo vertexInputStateCI;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI;
    VkPipelineTessellationStateCreateInfo tessellationStateCI;
    VkPipelineRasterizationStateCreateInfo rasterizationStateCI;
    VkPipelineMultisampleStateCreateInfo multisampleStateCI;
    VkPipelineDepthStencilStateCreateInfo depthStencilStateCI;
    VkPipelineColorBlendStateCreateInfo colorBlendStateCI;
    VkPipelineRenderingCreateInfo renderingCI;
    VkPipelineLayout layout;
};

} // namespace SYN::VK
