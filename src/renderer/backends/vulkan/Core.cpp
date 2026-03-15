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
    m_Instance = createInstance();

    m_DebugUtilsMessenger = createDebugMessenger(m_Instance);

    VkResult res{glfwCreateWindowSurface(m_Instance, window.getHandle(),
                                         nullptr, &m_Surface)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan surface");
    }

    m_Device = createDevice(m_Instance, m_Surface);
    m_Swapchain = createSwapchain(m_Device, m_Surface, window);

    {
        VkShaderModule vertShaderModule{
            createShaderModule(m_Device, "generated/shaders/first.vert.spv")};

        VkShaderModule fragShaderModule{
            createShaderModule(m_Device, "generated/shaders/first.frag.spv")};

        VkPipelineLayoutCreateInfo layoutCI{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        };
        vkCreatePipelineLayout(m_Device.logical, &layoutCI, nullptr,
                               &m_GraphicsPipelineLayout);
        GraphicsPipelineBuilder graphicsPipelineBuilder{};
        m_GraphicsPipeline =
            graphicsPipelineBuilder
                .setShaderStage(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT)
                .setShaderStage(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT)
                .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setFaceCulling(VK_CULL_MODE_BACK_BIT,
                                VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .setPolygonMode(VK_POLYGON_MODE_FILL)
                .addColorAttachment(m_Swapchain.format)
                .disableDepthTest()
                .disableMultisampling()
                .build(m_Device, m_GraphicsPipelineLayout);

        vkDestroyShaderModule(m_Device.logical, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_Device.logical, fragShaderModule, nullptr);
    }

    for (size_t i{}; i < c_MaxFramesInFlight; i++) {
        VkCommandPoolCreateInfo graphicsCmdPoolCI{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex =
                m_Device.queues[QueueFamily::graphics].familyIndex};
        VkCommandPool graphicsCmdPool{};
        vkCreateCommandPool(m_Device.logical, &graphicsCmdPoolCI, nullptr,
                            &graphicsCmdPool);

        VkCommandBufferAllocateInfo cmdBufferAllocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = graphicsCmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer graphicsCmdBuffer{};
        res = vkAllocateCommandBuffers(m_Device.logical, &cmdBufferAllocInfo,
                                       &graphicsCmdBuffer);
        if (res != VK_SUCCESS) {
            spdlog::error("Could not allocate command buffers, VkResult = {}",
                          static_cast<int>(res));
        }

        m_GraphicsCmdPools[i] = graphicsCmdPool;
        m_GraphicsCmdBuffers[i] = graphicsCmdBuffer;
    }
}

void VulkanBackend::render(Window &window) {}
void VulkanBackend::shutdown() {
    vkDeviceWaitIdle(m_Device.logical);

    for (size_t i{}; i < c_MaxFramesInFlight; i++) {
        VkCommandPool cmdPool{m_GraphicsCmdPools[i]};
        vkDestroyCommandPool(m_Device.logical, cmdPool, nullptr);
    }
    vkDestroyPipelineLayout(m_Device.logical, m_GraphicsPipelineLayout,
                            nullptr);
    vkDestroyPipeline(m_Device.logical, m_GraphicsPipeline, nullptr);

    destroySwapchain(&m_Swapchain, m_Device);
    destroyDevice(&m_Device);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    destroyDebugMessenger(m_Instance, m_DebugUtilsMessenger);
    vkDestroyInstance(m_Instance, nullptr);
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
