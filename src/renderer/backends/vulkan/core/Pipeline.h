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

    VkPipeline build(const Device &device, VkPipelineLayout layout,
                     std::span<VkPipelineShaderStageCreateInfo> shaderStageCIs);

    VkPipeline
    build(const Device &device, VkPipelineLayout layout,
          std::unordered_map<VkShaderStageFlagBits, std::string> shaderPaths);

    GraphicsPipelineBuilder &
    setInputAssembly(VkPrimitiveTopology topology,
                     VkBool32 primitiveRestartEnable = VK_FALSE);

    GraphicsPipelineBuilder &setFaceCulling(VkCullModeFlags mode,
                                            VkFrontFace frontFace);
    GraphicsPipelineBuilder &setPolygonMode(VkPolygonMode mode,
                                            float lineWidth = 1.f);

    GraphicsPipelineBuilder &addColorAttachment(VkFormat format,
                                                bool enableAlphaBlend);

    GraphicsPipelineBuilder &enableDepthAttachment();

    GraphicsPipelineBuilder &setMSAA(VkSampleCountFlagBits sampleCount);

  private:
    std::vector<VkPipelineColorBlendAttachmentState>
        m_ColorBlendAttachmentStates{};

    std::vector<VkFormat> m_ColorFormats{};
    VkFormat m_DepthFormat{};

    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyStateCI;
    VkPipelineRasterizationStateCreateInfo m_RasterizationStateCI;
    VkPipelineMultisampleStateCreateInfo m_MultisampleStateCI;
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilStateCI;
};

} // namespace SYN::VK
