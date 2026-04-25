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
    m_ColorBlendAttachmentStates.clear();

    m_InputAssemblyStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    };
    m_RasterizationStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    };
    // default disabled
    m_MultisampleStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT};
    // default disabled
    m_DepthStencilStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
    };
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
SYN::VK::GraphicsPipelineBuilder::addColorAttachment(VkFormat format,
                                                     bool enableAlphaBlend) {
    VkPipelineColorBlendAttachmentState blendState{
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
    };
    if (enableAlphaBlend) {
        blendState = VkPipelineColorBlendAttachmentState{
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask =
                VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };
    }

    m_ColorBlendAttachmentStates.emplace_back(blendState);
    m_ColorFormats.emplace_back(format);

    return *this;
}

SYN::VK::GraphicsPipelineBuilder &
SYN::VK::GraphicsPipelineBuilder::enableDepthWriting() {
    m_DepthStencilStateCI.depthWriteEnable = VK_TRUE;
    m_HasDepthAttachment = true;

    return *this;
}

SYN::VK::GraphicsPipelineBuilder &
SYN::VK::GraphicsPipelineBuilder::enableDepthTesting() {
    m_DepthStencilStateCI.depthTestEnable = VK_TRUE;
    m_HasDepthAttachment = true;

    return *this;
}

SYN::VK::GraphicsPipelineBuilder &
SYN::VK::GraphicsPipelineBuilder::setMSAA(VkSampleCountFlagBits sampleCount) {
    m_MultisampleStateCI.rasterizationSamples = sampleCount;
    return *this;
}

VkPipeline SYN::VK::GraphicsPipelineBuilder::build(
    const Device &device, VkPipelineLayout layout,
    std::span<VkPipelineShaderStageCreateInfo> shaderStageCIs) {

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

    if (m_HasDepthAttachment) {
        renderingCI.depthAttachmentFormat = m_DepthFormat;
    }

    VkGraphicsPipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &renderingCI,
        .stageCount = static_cast<uint32_t>(shaderStageCIs.size()),
        .pStages = shaderStageCIs.data(),
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

VkPipeline SYN::VK::GraphicsPipelineBuilder::build(
    const Device &device, VkPipelineLayout layout,
    std::unordered_map<VkShaderStageFlagBits, std::string> shaderPaths) {

    std::vector<VkShaderModule> shaderModules{};
    shaderModules.reserve(shaderPaths.size());
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCIs{};
    shaderStageCIs.reserve(shaderPaths.size());

    for (const auto &[stage, path] : shaderPaths) {
        VkShaderModule shaderModule{createShaderModule(device, path)};
        shaderModules.emplace_back(shaderModule);

        VkPipelineShaderStageCreateInfo shaderStageCI{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = stage,
            .module = shaderModule,
            .pName = "main",
        };
        shaderStageCIs.emplace_back(shaderStageCI);
    }

    VkPipeline pipeline{build(device, layout, shaderStageCIs)};

    for (auto shaderModule : shaderModules) {
        vkDestroyShaderModule(device.logical, shaderModule, nullptr);
    }

    return pipeline;
}

VkPipeline SYN::VK::buildComputePipeline(const Device &device,
                                         VkPipelineLayout layout,
                                         const std::string &shaderPath) {
    VkShaderModule shaderModule{createShaderModule(device, shaderPath)};

    VkPipelineShaderStageCreateInfo stageCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shaderModule,
        .pName = "main",
    };

    VkComputePipelineCreateInfo computeCI{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = stageCI,
        .layout = layout};

    VkPipeline pipeline{};
    VkResult res{vkCreateComputePipelines(device.logical, VK_NULL_HANDLE, 1,
                                          &computeCI, nullptr, &pipeline)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create compute pipeline");
    }

    vkDestroyShaderModule(device.logical, shaderModule, nullptr);

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
