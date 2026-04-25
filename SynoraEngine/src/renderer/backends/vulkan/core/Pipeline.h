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

    GraphicsPipelineBuilder &enableDepthTesting();
    GraphicsPipelineBuilder &enableDepthWriting();

    GraphicsPipelineBuilder &setMSAA(VkSampleCountFlagBits sampleCount);

  private:
    std::vector<VkPipelineColorBlendAttachmentState>
        m_ColorBlendAttachmentStates{};

    std::vector<VkFormat> m_ColorFormats{};
    VkFormat m_DepthFormat{VK_FORMAT_D32_SFLOAT};
    bool m_HasDepthAttachment{};

    VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyStateCI;
    VkPipelineRasterizationStateCreateInfo m_RasterizationStateCI;
    VkPipelineMultisampleStateCreateInfo m_MultisampleStateCI;
    VkPipelineDepthStencilStateCreateInfo m_DepthStencilStateCI;
};

VkPipeline buildComputePipeline(const Device &device, VkPipelineLayout layout,
                                const std::string &shaderPath);

} // namespace SYN::VK
