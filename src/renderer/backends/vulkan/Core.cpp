#include "Core.h"
#include "DebugMessenger.h"
#include "Instance.h"
#include "Swapchain.h"

#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

#include <PuzzleEngine/core/Window.h>

using namespace SYN;

namespace {
VkShaderModule createShaderModule(const Device &device,
                                  const std::string &path);
}

void SYN::VulkanBackend::init(Window &window) {
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

    VkPipelineShaderStageCreateInfo vertPipeCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
    };

    vkDestroyShaderModule(device.logical, vertShaderModule, nullptr);

    m_State = VulkanState{.instance = instance,
                          .surface = surface,
                          .debugUtilsMessenger = debugUtilsMessenger,
                          .device = device,
                          .swapchain = swapchain};
}
void SYN::VulkanBackend::shutdown() {
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
        return {};
    }
    std::streampos fileSize{file.tellg()};
    if (fileSize < 0) {
        spdlog::error("Tellg failed, shader module creation failed", path);
        assert(false);
        return {};
    }

    if ((fileSize % sizeof(uint32_t)) != 0) {
        spdlog::error("Could not create shader module, {} file size was not a "
                      "multiple of 32 "
                      "and may be malformed",
                      path);
        assert(false);
        return {};
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
        return {};
    }
    return shaderModule;
}
} // namespace
