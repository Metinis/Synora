#include "Core.h"
#include "DebugMessenger.h"
#include "Instance.h"
#include "Pipeline.h"
#include "Swapchain.h"

#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

#include <PuzzleEngine/core/Window.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

namespace {
VkShaderModule createShaderModule(const Device &device,
                                  const std::string &path);
}

void VulkanBackend::init(SYN::Window &window) {
    VkInstance instance{createInstance()};

    VkDebugUtilsMessengerEXT debugUtilsMessenger{
        createDebugMessenger(instance)};

    VkSurfaceKHR surface{};
    VkResult res{glfwCreateWindowSurface(instance, window.getHandle(), nullptr,
                                         &surface)};
    if (res != VK_SUCCESS) {
        spdlog::info("Could not create Vulkan surface");
    }

    Device device{createDevice(instance, surface)};
    Swapchain swapchain{createSwapchain(device, surface, window)};

    VkShaderModule vertShaderModule{
        createShaderModule(device, "generated/shaders/first.vert.spv")};

    VkShaderModule fragShaderModule{
        createShaderModule(device, "generated/shaders/first.frag.spv")};

    VkPipelineShaderStageCreateInfo vertPipeCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragPipeCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main",
    };
    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStageCIs{vertPipeCI,
                                                                  fragPipeCI};

    VkPipelineLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };
    VkPipelineLayout layout;
    vkCreatePipelineLayout(device.logical, &layoutCI, nullptr, &layout);

    GraphicsPipelineBuilder graphicsPipelineBuilder{};
    VkPipeline graphicsPipeline{
        graphicsPipelineBuilder
            .setShaderStage(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT)
            .setShaderStage(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT)
            .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
            .setFaceCulling(VK_CULL_MODE_BACK_BIT,
                            VK_FRONT_FACE_COUNTER_CLOCKWISE)
            .setPolygonMode(VK_POLYGON_MODE_FILL)
            .addColorAttachment(swapchain.format)
            .disableDepthTest()
            .disableMultisampling()
            .build(device, layout)};

    vkDestroyShaderModule(device.logical, vertShaderModule, nullptr);

    m_State = VulkanState{.instance = instance,
                          .surface = surface,
                          .debugUtilsMessenger = debugUtilsMessenger,
                          .device = std::move(device),
                          .swapchain = std::move(swapchain),
                          .graphicsPipelineLayout = layout,
                          .graphicsPipeline = graphicsPipeline};
}
void VulkanBackend::shutdown() {
    vkDestroyPipelineLayout(m_State.device.logical,
                            m_State.graphicsPipelineLayout, nullptr);
    vkDestroyPipeline(m_State.device.logical, m_State.graphicsPipeline,
                      nullptr);

    destroySwapchain(&m_State.swapchain, m_State.device);
    destroyDevice(&m_State.device);
    vkDestroySurfaceKHR(m_State.instance, m_State.surface, nullptr);
    destroyDebugMessenger(m_State.instance, m_State.debugUtilsMessenger);
    vkDestroyInstance(m_State.instance, nullptr);

    m_State = {};
}

// TODO: add default shaders for if theres an error
namespace {
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
