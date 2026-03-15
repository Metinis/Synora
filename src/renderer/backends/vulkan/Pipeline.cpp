#include "Pipeline.h"
#include "Device.h"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

SYN::GraphicsPipeline::GraphicsPipeline() { reset(); }

void SYN::GraphicsPipeline::reset() {
    this->shaderStageCIs = {};
    this->vertexInputStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    this->inputAssemblyStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    this->rasterizationStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    this->multisampleStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    this->depthStencilStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    this->colorBlendStateCI = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    this->renderingCI = {.sType =
                             VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    this->layout = {VK_NULL_HANDLE};
}

VkPipeline SYN::GraphicsPipeline::build(const Device &device) {
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
            static_cast<uint32_t>(this->colorBlendAttachmentStates.size()),
        .pAttachments = this->colorBlendAttachmentStates.data(),
    };

    VkGraphicsPipelineCreateInfo pipelineCI{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &this->renderingCI,
        .stageCount = static_cast<uint32_t>(this->shaderStageCIs.size()),
        .pStages = this->shaderStageCIs.data(),
        .pVertexInputState = &this->vertexInputStateCI,
        .pInputAssemblyState = &this->inputAssemblyStateCI,
        .pTessellationState = nullptr,
        .pViewportState = &viewportCI,
        .pRasterizationState = &this->rasterizationStateCI,
        .pMultisampleState = &this->multisampleStateCI,
        .pDepthStencilState = &this->depthStencilStateCI,
        .pColorBlendState = &this->colorBlendStateCI,
        .pDynamicState = &dynamicCI,
        .layout = this->layout,
    };
    VkPipeline pipeline{};
    VkResult res{vkCreateGraphicsPipelines(device.logical, VK_NULL_HANDLE, 1,
                                           &pipelineCI, nullptr, &pipeline)};

    if (res != VK_SUCCESS) {
        spdlog::error("Could not create graphics pipeline");
    }
    return pipeline;
}
