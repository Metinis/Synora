#include "DebugMessenger.h"

#include <spdlog/spdlog.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;

namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerUserCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

VkResult vkCreateDebugUtilsMessengerThunk(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger);

void vkDestroyDebugUtilsMessengerThunk(VkInstance instance,
                                       VkDebugUtilsMessengerEXT debugMessenger,
                                       const VkAllocationCallbacks *pAllocator);

constexpr VkDebugUtilsMessengerCreateInfoEXT c_DebugUtilsMessengerCI{
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
    .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
    .pfnUserCallback = debugMessengerUserCallback,
};

} // namespace

#ifdef NDEBUG
VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance instance) {
    return {};
}

void destroyDebugMessenger(VkInstance instance,
                           VkDebugUtilsMessengerEXT debugMessenger) {}

const VkDebugUtilsMessengerCreateInfoEXT *getDebugMessengerCreateInfo() {
    return nullptr;
}

#else

VkDebugUtilsMessengerEXT SYN::createDebugMessenger(VkInstance instance) {
    VkDebugUtilsMessengerEXT debugMessenger{};
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCI{
        *getDebugMessengerCreateInfo()};
    VkResult res{vkCreateDebugUtilsMessengerThunk(
        instance, &debugUtilsMessengerCI, nullptr, &debugMessenger)};
    if (res != VK_SUCCESS) {
        spdlog::warn("Could not create Vulkan debug manager: VkResult = ",
                     static_cast<int>(res));
    }

    return debugMessenger;
}

void SYN::destroyDebugMessenger(VkInstance instance,
                                VkDebugUtilsMessengerEXT debugMessenger) {
    vkDestroyDebugUtilsMessengerThunk(instance, debugMessenger, nullptr);
}

const VkDebugUtilsMessengerCreateInfoEXT *SYN::getDebugMessengerCreateInfo() {
    return &c_DebugUtilsMessengerCI;
}

#endif

namespace {
VKAPI_ATTR VkBool32 VKAPI_CALL debugMessengerUserCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
    void *pUserData) {

    spdlog::level::level_enum logLevel{};
    switch (messageSeverity) {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        logLevel = spdlog::level::level_enum::trace;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        logLevel = spdlog::level::level_enum::info;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        logLevel = spdlog::level::level_enum::warn;
        break;
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        logLevel = spdlog::level::level_enum::err;
        break;
    default:
        break;
    }
    if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        spdlog::log(logLevel, "[VULKAN] {}", pCallbackData->pMessage);
    }

    return false;
}

VkResult vkCreateDebugUtilsMessengerThunk(
    VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger) {

    auto vkCreateDebugUtilsMessenger{
        reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"))};

    if (!vkCreateDebugUtilsMessenger) {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    return vkCreateDebugUtilsMessenger(instance, pCreateInfo, pAllocator,
                                       pDebugMessenger);
}

void vkDestroyDebugUtilsMessengerThunk(
    VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator) {

    auto vkDestroyDebugUtilsMessenger{
        reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(instance,
                                  "vkDestroyDebugUtilsMessengerEXT"))};

    if (!vkDestroyDebugUtilsMessenger) {
        spdlog::warn("vkDestroyDebugUtilsMessengerEXT not present while "
                     "vkDestroyDebugUtilsMessengerEXT called");
        return;
    }

    vkDestroyDebugUtilsMessenger(instance, debugMessenger, pAllocator);
}
} // namespace
