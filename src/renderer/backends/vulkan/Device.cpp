#include "Device.h"

#include <optional>
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

namespace {

VkPhysicalDevice createPhysicalDevice(VkInstance instance,
                                      VkSurfaceKHR surface);
}

SYN::Device SYN::createDevice(VkInstance instance, VkSurfaceKHR surface) {
    VkPhysicalDevice physicalDevice{createPhysicalDevice(instance, surface)};

    VkPhysicalDeviceVulkan12Features features12{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .descriptorIndexing = VK_TRUE,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageBufferArrayNonUniformIndexing = VK_TRUE,
        .shaderStorageImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingUniformBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingSampledImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageImageUpdateAfterBind = VK_TRUE,
        .descriptorBindingStorageBufferUpdateAfterBind = VK_TRUE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
        .bufferDeviceAddress = VK_TRUE,
    };

    VkPhysicalDeviceVulkan13Features features13{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &features12,
        .synchronization2 = VK_TRUE,
        .dynamicRendering = VK_TRUE,
    };

    VkPhysicalDeviceFeatures2 physicalDeviceFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features13,
        .features = {.samplerAnisotropy = VK_TRUE}};

    VkDeviceCreateInfo deviceCI{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &physicalDeviceFeatures,
    };

    VkDevice device{};
    VkResult res{vkCreateDevice(physicalDevice, &deviceCI, nullptr, &device)};

    if (res != VK_SUCCESS) {
        spdlog::error(
            "Vulkan logical device could not be created. VkResult = {}",
            static_cast<int>(res));
    }

    return Device{.physical = physicalDevice, .logical = device};
}

void SYN::destroyDevice(Device *device) {
    vkDestroyDevice(device->logical, nullptr);
    device->logical = {};
    device->physical = {};
}

namespace {
VkPhysicalDevice createPhysicalDevice(VkInstance instance,
                                      VkSurfaceKHR surface) {
    uint32_t physicalDeviceCount{};
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance, &physicalDeviceCount,
                               physicalDevices.data());

    VkPhysicalDevice suitablePhysicalDevice{};
    uint32_t maxDeviceScore{};
    bool allFeaturesSupported{};
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
        bool featuresSupported{features.features.samplerAnisotropy == VK_TRUE};
        bool features12Supported{
            features12.descriptorIndexing &&
            features12.shaderSampledImageArrayNonUniformIndexing &&
            features12.shaderStorageBufferArrayNonUniformIndexing &&
            features12.shaderStorageImageArrayNonUniformIndexing &&
            features12.descriptorBindingUniformBufferUpdateAfterBind &&
            features12.descriptorBindingSampledImageUpdateAfterBind &&
            features12.descriptorBindingStorageImageUpdateAfterBind &&
            features12.descriptorBindingStorageBufferUpdateAfterBind &&
            features12.descriptorBindingPartiallyBound &&
            features12.runtimeDescriptorArray &&
            features12.bufferDeviceAddress};

        bool features13Supported{features13.synchronization2 &&
                                 features13.dynamicRendering};

        uint32_t currentDeviceScore{};

        if (!(featuresSupported && features12Supported &&
              features13Supported)) {
            continue;
        }
        allFeaturesSupported = true;

        switch (physicalDeviceProps.deviceType) {
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
            currentDeviceScore += 10000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
            currentDeviceScore += 1000;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_CPU:
            currentDeviceScore += 100;
            break;
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
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

    if (!allFeaturesSupported) {
        spdlog::error("Some Vulkan feature are not supported any "
                      "physical device");
    }

    if (suitablePhysicalDevice == 0) {
        spdlog::error("Vulkan physical device was not created");
    }

    return suitablePhysicalDevice;
}

} // namespace
