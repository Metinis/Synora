#include "Instance.h"
#include "DebugMessenger.h"
#include "Extensions.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;

namespace {
std::vector<const char *>
findLayers(std::span<const char *> requiredLayers,
           std::span<VkLayerProperties> availableLayerProps);
} // namespace

VkInstance SYN::createInstance() {
    if (glfwVulkanSupported() != GLFW_TRUE) {
        spdlog::error(
            "Vulkan backend is being used while Vulkan is not supported");
        assert(false);
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

    std::vector<const char *> requiredExtensions(
        windowExtensions, windowExtensions + windowExtensionsCount);

#ifdef NDEBUG
    std::array<const char *, 0> requiredLayers{};
    const VkDebugUtilsMessengerCreateInfoEXT *debugUtilsMessengerCI{nullptr};
#else
    std::array<const char *, 1> requiredLayers{"VK_LAYER_KHRONOS_validation"};
    requiredExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    const VkDebugUtilsMessengerCreateInfoEXT *debugUtilsMessengerCI{
        getDebugMessengerCreateInfo()};
#endif

    VkInstanceCreateFlags instanceFlags{};

#ifdef __APPLE__
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
    instanceFlags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    uint32_t availableExtensionPropCount{};
    vkEnumerateInstanceExtensionProperties(
        nullptr, &availableExtensionPropCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensionProps(
        availableExtensionPropCount);
    vkEnumerateInstanceExtensionProperties(
        nullptr, &availableExtensionPropCount, availableExtensionProps.data());

    std::vector<const char *> extensions{
        findExtensions(requiredExtensions, availableExtensionProps)};

    uint32_t availableLayerPropCount{};
    vkEnumerateInstanceLayerProperties(&availableLayerPropCount, nullptr);
    std::vector<VkLayerProperties> availableLayerProps(availableLayerPropCount);
    vkEnumerateInstanceLayerProperties(&availableLayerPropCount,
                                       availableLayerProps.data());

    std::vector<const char *> layers{
        findLayers(requiredLayers, availableLayerProps)};

    VkInstanceCreateInfo vkInstanceCI{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = debugUtilsMessengerCI,
        .flags = instanceFlags,
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkInstance instance{};
    VkResult res{vkCreateInstance(&vkInstanceCI, nullptr, &instance)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan instance");
        assert(false);
    }

    return instance;
}

namespace {

std::vector<const char *>
findLayers(std::span<const char *> requiredLayers,
           std::span<VkLayerProperties> availableLayerProps) {
    std::vector<const char *> foundLayers{};

    for (const auto &layer : requiredLayers) {
        auto itt{
            std::find_if(availableLayerProps.begin(), availableLayerProps.end(),
                         [&](VkLayerProperties layerProp) {
                             return strcmp(layer, layerProp.layerName) == 0;
                         })};

        if (itt != availableLayerProps.end()) {
            foundLayers.emplace_back(itt->layerName);
        } else {
            spdlog::warn("Could not find Vulkan layer {}", layer);
        }
    }

    return foundLayers;
}
} // namespace
