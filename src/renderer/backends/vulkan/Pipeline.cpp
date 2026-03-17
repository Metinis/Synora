#include "Pipeline.h"
#include "Device.h"
#include <fstream>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

namespace {
VkShaderModule createShaderModule(const Device &device,
                                  const std::string &path);
}

SYN::VK::GraphicsPipelineBuilder::GraphicsPipelineBuilder() { reset(); }

void SYN::VK::GraphicsPipelineBuilder::reset() {
    m_ShaderStageCIs.clear();
    m_ColorBlendAttachmentStates.clear();

    m_InputAssemblyStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    };
    m_RasterizationStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    m_MultisampleStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    m_DepthStencilStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
}

GraphicsPipelineBuilder &
SYN::VK::GraphicsPipelineBuilder::setShaderStage(VkShaderModule module,
                                                 VkShaderStageFlagBits stage) {
    VkPipelineShaderStageCreateInfo stageCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = module,
        .pName = "main",
    };

    m_ShaderStageCIs.emplace_back(stageCI);

    return *this;
}

GraphicsPipelineBuilder &SYN::VK::GraphicsPipelineBuilder::setInputAssembly(
    VkPrimitiveTopology topology, VkBool32 primitiveRestartEnable) {
    m_InputAssemblyStateCI.topology = topology;
    m_InputAssemblyStateCI.primitiveRestartEnable = primitiveRestartEnable;

    return *this;
}

GraphicsPipelineBuilder &
SYN::VK::GraphicsPipelineBuilder::setFaceCulling(VkCullModeFlags mode,
                                                 VkFrontFace frontFace) {
    m_RasterizationStateCI.cullMode = mode;
    m_RasterizationStateCI.frontFace = frontFace;

    return *this;
}

GraphicsPipelineBuilder &
SYN::VK::GraphicsPipelineBuilder::setPolygonMode(VkPolygonMode mode,
                                                 float lineWidth) {
    m_RasterizationStateCI.polygonMode = mode;
    m_RasterizationStateCI.lineWidth = lineWidth;

    return *this;
}

SYN::VK::GraphicsPipelineBuilder &
SYN::VK::GraphicsPipelineBuilder::addColorAttachment(VkFormat format) {
    VkPipelineColorBlendAttachmentState blendState{
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };

    m_ColorBlendAttachmentStates.emplace_back(blendState);
    m_ColorFormats.emplace_back(format);

    return *this;
}

GraphicsPipelineBuilder &
SYN::VK::GraphicsPipelineBuilder::disableMultisampling() {
    m_MultisampleStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.f};

    return *this;
}
GraphicsPipelineBuilder &SYN::VK::GraphicsPipelineBuilder::disableDepthTest() {
    m_DepthStencilStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthCompareOp = VK_COMPARE_OP_NEVER,
        .maxDepthBounds = 1.f};

    return *this;
}

VkPipeline SYN::VK::GraphicsPipelineBuilder::build(const Device &device,
                                                   VkPipelineLayout layout) {

    VkPipelineVertexInputStateCreateInfo vertexInputStateCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineViewportStateCreateInfo viewportCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1};

    std::array<VkDynamicState, 2> dynamicStates{VK_DYNAMIC_STATE_SCISSOR,
                                                VK_DYNAMIC_STATE_VIEWPORT};

    VkPipelineDynamicStateCreateInfo dynamicCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStates.size(),
        .pDynamicStates = dynamicStates.data(),
    };

    VkPipelineColorBlendStateCreateInfo colorBlendCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount =
            static_cast<uint32_t>(m_ColorBlendAttachmentStates.size()),
        .pAttachments = m_ColorBlendAttachmentStates.data(),
    };

    VkPipelineRenderingCreateInfo renderingCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = static_cast<uint32_t>(m_ColorFormats.size()),
        .pColorAttachmentFormats = m_ColorFormats.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCI,
        .stageCount = static_cast<uint32_t>(m_ShaderStageCIs.size()),
        .pStages = m_ShaderStageCIs.data(),
        .pVertexInputState = &vertexInputStateCI,
        .pInputAssemblyState = &m_InputAssemblyStateCI,
        .pTessellationState = nullptr,
        .pViewportState = &viewportCI,
        .pRasterizationState = &m_RasterizationStateCI,
        .pMultisampleState = &m_MultisampleStateCI,
        .pDepthStencilState = &m_DepthStencilStateCI,
        .pColorBlendState = &colorBlendCI,
        .pDynamicState = &dynamicCI,
        .layout = layout,
    };
    VkPipeline pipeline{};
    VkResult res{vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1,
                                           &pipelineCI, nullptr, &pipeline)};

    if (res != VK_SUCCESS) {
        spdlog::error("Could not create graphics pipeline");
    }
    return pipeline;
}

VkPipeline SYN::VK::makeFirstPipeline(
    const Device &device, VkPipelineLayout layout,
    std::unordered_map<VkShaderStageFlagBits, std::string> shaderPaths,
    VkFormat renderTargetColorFormat) {
    GraphicsPipelineBuilder pipelineBuilder{};
    pipelineBuilder.setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .setFaceCulling(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .setPolygonMode(VK_POLYGON_MODE_FILL)
        .addColorAttachment(renderTargetColorFormat)
        .disableDepthTest()
        .disableMultisampling();

    std::vector<VkShaderModule> shaderModules{};
    shaderModules.reserve(shaderPaths.size());

    for (const auto &[stage, path] : shaderPaths) {

        VkShaderModule shaderModule{createShaderModule(device, path)};
        shaderModules.emplace_back(shaderModule);

        pipelineBuilder.setShaderStage(shaderModule, stage);
    }

    VkPipeline pipeline{pipelineBuilder.build(device, layout)};

    for (auto shaderModule : shaderModules) {
        vkDestroyShaderModule(device.logical, shaderModule, nullptr);
    }

    return pipeline;
}

namespace {

// TODO: add default shaders for if theres an error
VkShaderModule createShaderModule(const Device &device,
                                  const std::string &path) {
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        spdlog::error("Could not open {}, shader module creation failed", path);
        assert(false);
        return VK_NULL_HANDLE;
    }
    std::streampos fileSize{file.tellg()};
    if (fileSize < 0) {
        spdlog::error("Tellg failed, shader module creation failed", path);
        assert(false);
        return VK_NULL_HANDLE;
    }

    if ((fileSize % sizeof(uint32_t)) != 0) {
        spdlog::error("Could not create shader module, {} file size was not a "
                      "multiple of 32 "
                      "and may be malformed",
                      path);
        assert(false);
        return VK_NULL_HANDLE;
    }

    std::vector<uint32_t> shaderCode(fileSize / sizeof(uint32_t));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(shaderCode.data()), fileSize);

    VkShaderModuleCreateInfo shaderModuleCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderCode.size() * sizeof(uint32_t), // byte size
        .pCode = shaderCode.data()};

    VkShaderModule shaderModule{};
    VkResult res{vkCreateShaderModule(device.logical, &shaderModuleCI, nullptr,
                                      &shaderModule)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create shader module of {},  VkResult = {}",
                      path, static_cast<int>(res));
        assert(false);
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}
} // namespace
