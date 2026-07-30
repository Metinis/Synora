#include "Device.h"
#include "Extensions.h"

#include <optional>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

namespace {

struct QueueFamilyInfo {
    std::unordered_map<QueueFamily, uint32_t> indices;
    std::unordered_map<uint32_t, uint32_t> indexToCount;
    uint32_t maxFamilyQueueCount;
};

VkPhysicalDevice selectPhysicalDevice(VkInstance instance,
                                      VkSurfaceKHR surface);

VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice,
                             const QueueFamilyInfo &queueFamilyInfo,
                             const VkPhysicalDeviceFeatures2 &features);

std::unordered_map<QueueFamily, Queue>
getDeviceQueues(VkDevice device, const QueueFamilyInfo &queueFamilyInfo);

QueueFamilyInfo findQueueFamilyIndices(VkPhysicalDevice physicalDevice,
                                       VkSurfaceKHR surface);
} // namespace

Device VK::createDevice(VkInstance instance, VkSurfaceKHR surface) {
    VkPhysicalDeviceVulkan12Features features12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
        //.descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE, //sry my gtx1070ti doesnt support
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .timelineSemaphore = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &features12,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    VkPhysicalDeviceFeatures2 features{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features13,
        .features = {
            .fillModeNonSolid = VK_TRUE,
            .samplerAnisotropy = VK_TRUE,
        }};

    VkPhysicalDevice physicalDevice{selectPhysicalDevice(instance, surface)};

    QueueFamilyInfo queueFamilyInfo{
        findQueueFamilyIndices(physicalDevice, surface)};

    VkDevice device{
        createLogicalDevice(physicalDevice, queueFamilyInfo, features)};

    std::unordered_map<QueueFamily, Queue> queues{
        getDeviceQueues(device, queueFamilyInfo)};

    VkPhysicalDeviceProperties deviceProps{};
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProps);

    return Device{.physical = physicalDevice,
                  .logical = device,
                  .queues = queues,
                  .properties = deviceProps};
}

void VK::destroyDevice(Device &device) {
    vkDestroyDevice(device.logical, nullptr);
    device.logical = {};
    device.physical = {};
}

namespace {
VkPhysicalDevice selectPhysicalDevice(VkInstance instance,
                                      VkSurfaceKHR surface) {
    uint32_t physicalDeviceCount{};
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
                               physicalDevices.data());

    VkPhysicalDevice suitablePhysicalDevice{};
    uint32_t maxDeviceScore{};
    for (const auto &physicalDevice : physicalDevices) {
        VkPhysicalDeviceProperties physicalDeviceProps{};
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProps);

        VkPhysicalDeviceVulkan12Features features12{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};

        VkPhysicalDeviceVulkan13Features features13{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
            .pNext = &features12};

        VkPhysicalDeviceFeatures2 features{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = &features13};

        vkGetPhysicalDeviceFeatures2(physicalDevice, &features);

        // TODO: pass in features and compare them against what are actually
        // supported
        bool featuresSupported{features.features.samplerAnisotropy == VK_TRUE &&
                               features.features.fillModeNonSolid == VK_TRUE};
        bool features12Supported{
            features12.descriptorIndexing &&
            features12.shaderSampledImageArrayNonUniformIndexing &&
            features12.shaderStorageBufferArrayNonUniformIndexing &&
            features12.shaderStorageImageArrayNonUniformIndexing &&
            //features12.descriptorBindingUniformBufferUpdateAfterBind && sry my 1070ti doesn't support
            features12.descriptorBindingSampledImageUpdateAfterBind &&
            features12.descriptorBindingStorageImageUpdateAfterBind &&
            features12.descriptorBindingStorageBufferUpdateAfterBind &&
            features12.descriptorBindingPartiallyBound &&
            features12.runtimeDescriptorArray && features12.timelineSemaphore &&
            features12.bufferDeviceAddress};

        bool features13Supported{features13.synchronization2 &&
                                 features13.dynamicRendering};

        // supporting all features is a requirement for a device to be selected
        // atleast for now
        if (!(featuresSupported && features12Supported &&
              features13Supported)) {
            continue;
        }

        uint32_t currentDeviceScore{};
        switch (physicalDeviceProps.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            spdlog::debug("Found discrete GPU");
            currentDeviceScore += 10000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            spdlog::debug("Found integrated GPU");
            currentDeviceScore += 1000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            spdlog::debug("Found CPU");
            currentDeviceScore += 100;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
            spdlog::debug("Found Virtual GPU");
            currentDeviceScore += 10;
            break;
        default:
            break;
        }

        if (currentDeviceScore > maxDeviceScore) {
            maxDeviceScore = currentDeviceScore;
            suitablePhysicalDevice = physicalDevice;
        }
    }

    if (suitablePhysicalDevice == VK_NULL_HANDLE) {
        spdlog::error("Vulkan physical device was not created");
        assert(false);
    }

    return suitablePhysicalDevice;
}

QueueFamilyInfo findQueueFamilyIndices(VkPhysicalDevice physicalDevice,
                                       VkSurfaceKHR surface) {
    uint32_t queueFamilyPropsCount{};
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &queueFamilyPropsCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProps(
        queueFamilyPropsCount);

    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice, &queueFamilyPropsCount, queueFamilyProps.data());

    std::unordered_map<QueueFamily, uint32_t> indices;
    std::unordered_map<uint32_t, uint32_t> indexToCount;

    uint32_t maxFamilyQueueCount{};

    // TODO: add support for multiple queues per family
    uint32_t nQueueFamilies{static_cast<uint32_t>(QueueFamily::count)};
    for (uint32_t i{}; i < queueFamilyProps.size(); i++) {
        VkQueueFamilyProperties props{queueFamilyProps[i]};
        maxFamilyQueueCount = std::max(maxFamilyQueueCount, props.queueCount);

        VkBool32 surfaceSupported{};
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface,
                                             &surfaceSupported);

        if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
            !indices.contains(QueueFamily::graphics)) {
            indices[QueueFamily::graphics] = i;
            // setting it to 1 because future if statements dont correspond to
            // more queues, theyre all the same queue, so only 1 count
            indexToCount[i] = 1;
        }

        if (surfaceSupported && !indices.contains(QueueFamily::present)) {
            indices[QueueFamily::present] = i;
            indexToCount[i] = 1;
        }

        if (props.queueFlags & VK_QUEUE_COMPUTE_BIT &&
            !indices.contains(QueueFamily::compute)) {
            indices[QueueFamily::compute] = i;
            indexToCount[i] = 1;
        }

        if (props.queueFlags & VK_QUEUE_TRANSFER_BIT &&
            !indices.contains(QueueFamily::transfer)) {
            indices[QueueFamily::transfer] = i;
            indexToCount[i] = 1;
        }

        if (indices.size() >= nQueueFamilies) {
            break;
        }
    }

    if (indices.size() < nQueueFamilies) {
        spdlog::warn(
            "Physical device does not support creating all required queues");
    }

    return QueueFamilyInfo{.indices = indices,
                           .indexToCount = indexToCount,
                           .maxFamilyQueueCount = maxFamilyQueueCount};
}

VkDevice createLogicalDevice(VkPhysicalDevice physicalDevice,
                             const QueueFamilyInfo &queueFamilyInfo,
                             const VkPhysicalDeviceFeatures2 &features) {

    std::vector<const char *> requiredExtensions{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};

#ifdef __APPLE__
    requiredExtensions.emplace_back(VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
#endif

    uint32_t supportedExtensionsCount{};
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                         &supportedExtensionsCount, nullptr);
    std::vector<VkExtensionProperties> supportedExtensionProps(
        supportedExtensionsCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr,
                                         &supportedExtensionsCount,
                                         supportedExtensionProps.data());

    std::vector<const char *> extensions{
        findExtensions(requiredExtensions, supportedExtensionProps)};

    std::vector<float> priorities(queueFamilyInfo.maxFamilyQueueCount, 0.f);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    for (const auto &[index, count] : queueFamilyInfo.indexToCount) {
        VkDeviceQueueCreateInfo queueCI{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = index,
            .queueCount = count,
            .pQueuePriorities = priorities.data()};
        queueCreateInfos.emplace_back(queueCI);
    }

    VkDeviceCreateInfo deviceCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &features,
        .queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size()),
        .pQueueCreateInfos = queueCreateInfos.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    VkDevice device{};
    VkResult res{vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device)};

    if (res != VK_SUCCESS) {
        spdlog::error(
            "Vulkan logical device could not be created. VkResult = {}",
            static_cast<int>(res));
        assert(false);
    }

    return device;
}

std::unordered_map<QueueFamily, Queue>
getDeviceQueues(VkDevice device, const QueueFamilyInfo &queueFamilyInfo) {
    std::unordered_map<QueueFamily, Queue> queues{};
    for (const auto &[family, index] : queueFamilyInfo.indices) {
        VkQueue queueHandle{};
        vkGetDeviceQueue(device, index, 0, &queueHandle);
        Queue queue{
            .handle = queueHandle,
            .familyIndex = index,
        };

        queues[family] = queue;
    }
    return queues;
}

} // namespace
