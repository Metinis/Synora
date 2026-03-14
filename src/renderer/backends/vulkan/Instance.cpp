#include "Instance.h"
#include "DebugMessenger.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;

VkInstance SYN::createInstance() {
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

    uint32_t windowExtensionsCount{};
    const char **windowExtensions{
        glfwGetRequiredInstanceExtensions(&windowExtensionsCount)};

    std::vector<const char *> extensions(
        windowExtensions, windowExtensions + windowExtensionsCount);

#ifdef NDEBUG
    std::array<const char *, 0> layers{};
#else
    std::array<const char *, 1> layers{"VK_LAYER_KHRONOS_validation"};
    extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

#ifdef __APPLE__
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    const VkDebugUtilsMessengerCreateInfoEXT *debugUtilsMessengerCI{
        getDebugMessengerCreateInfo()};

    VkInstanceCreateInfo vkInstanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = debugUtilsMessengerCI,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkInstance instance{};
    VkResult res{vkCreateInstance(&vkInstanceCI, nullptr, &instance)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan instance");
    }

    return instance;
}
