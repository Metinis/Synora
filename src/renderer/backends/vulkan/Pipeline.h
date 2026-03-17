#pragma once

#include <span>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK {
struct Device;

class GraphicsPipelineBuilder {
  public:
    GraphicsPipelineBuilder();
    ~GraphicsPipelineBuilder() = default;

    void reset();
    VkPipeline build(const Device &device, VkPipelineLayout layout);

    GraphicsPipelineBuilder &setShaderStage(VkShaderModule module,
                                            VkShaderStageFlagBits stage);

    GraphicsPipelineBuilder &
    setInputAssembly(VkPrimitiveTopology topology,
                     VkBool32 primitiveRestartEnable = VK_FALSE);

    GraphicsPipelineBuilder &setFaceCulling(VkCullModeFlags mode,
                                            VkFrontFace frontFace);
    GraphicsPipelineBuilder &setPolygonMode(VkPolygonMode mode,
                                            float lineWidth = 1.f);

    GraphicsPipelineBuilder &addColorAttachment(VkFormat format);

    GraphicsPipelineBuilder &disableMultisampling();
    GraphicsPipelineBuilder &disableDepthTest();

  private:
    std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStageCIs{};
    std::vector<VkPipelineColorBlendAttachmentState>
        m_ColorBlendAttachmentStates{};

    std::vector<VkFormat> m_ColorFormats{};

    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyStateCI;
    VkPipelineRasterizationStateCreateInfo m_RasterizationStateCI;
    VkPipelineMultisampleStateCreateInfo m_MultisampleStateCI;
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilStateCI;
};

VkPipeline makeFirstPipeline(
    const Device &device, VkPipelineLayout layout,
    std::unordered_map<VkShaderStageFlagBits, std::string> shaderPaths,
    VkFormat renderTargetColorFormat);

} // namespace SYN::VK
