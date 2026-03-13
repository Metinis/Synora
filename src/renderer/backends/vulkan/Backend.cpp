#include "Backend.h"
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <PuzzleEngine/core/Window.h>

using namespace SYN;

void SYN::VulkanBackend::init(Window &window) {
    if (glfwVulkanSupported() != GLFW_TRUE) {
        spdlog::error(
            "Vulkan backend is being used while Vulkan is not supported");
    }

    VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                              .pApplicationName = "PuzzleEngine",
                              .applicationVersion =
                                  VK_MAKE_API_VERSION(0, 1, 0, 0),
                              .pEngineName = "PuzzleEngine",
                              .engineVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
                              .apiVersion = VK_API_VERSION_1_3};

    uint32_t rawExtensionCount{};
    const char **rawExtensions{
        glfwGetRequiredInstanceExtensions(&rawExtensionCount)};

    std::vector<const char *> extensions(rawExtensions,
                                         rawExtensions + rawExtensionCount);

    VkInstanceCreateInfo vkInstanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkInstance instance{};
    VkResult res{vkCreateInstance(&vkInstanceCI, nullptr, &instance)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan instance");
    }

    VkSurfaceKHR surface{};
    res = glfwCreateWindowSurface(instance, window.getHandle(), nullptr,
                                  &surface);
    if (res != VK_SUCCESS) {
        spdlog::info("Could not create Vulkan surface");
    }

    m_State = VulkanState{.instance = instance, .surface = surface};
}
void SYN::VulkanBackend::shutdown() {
    vkDestroySurfaceKHR(m_State.instance, m_State.surface, nullptr);
    vkDestroyInstance(m_State.instance, nullptr);
}
