#include "Swapchain.h"
#include "Device.h"

#include <GLFW/glfw3.h>
#include <PuzzleEngine/core/Window.h>
#include <spdlog/spdlog.h>

#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

namespace {
VkSurfaceFormatKHR findBestSurfaceFormat(const Device &device,
                                         VkSurfaceKHR surface);

VkPresentModeKHR findBestPresentMode(const Device &device,
                                     VkSurfaceKHR surface);
VkExtent2D
calcSurfaceExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities,
                  SYN::Window &window);
} // namespace

Swapchain SYN::VK::createSwapchain(const Device &device, VkSurfaceKHR surface,
                                   SYN::Window &window,
                                   VkSwapchainKHR oldSwapchain) {

    VkSurfaceCapabilitiesKHR surfaceCapabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physical, surface,
                                              &surfaceCapabilities);
    VkSurfaceFormatKHR surfaceFormat{findBestSurfaceFormat(device, surface)};
    VkPresentModeKHR presentMode{findBestPresentMode(device, surface)};
    VkExtent2D surfaceExtent{calcSurfaceExtent(surfaceCapabilities, window)};

    uint32_t minImageCount{};
    if (surfaceCapabilities.maxImageCount == 0) { // == 0 means no limit
        minImageCount = surfaceCapabilities.minImageCount + 1;
    } else {
        minImageCount = std::min(surfaceCapabilities.minImageCount + 1,
                                 surfaceCapabilities.maxImageCount);
    }

    VkSwapchainCreateInfoKHR swapchainCI{
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = minImageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = surfaceExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presentMode,
        .clipped = VK_TRUE,
        .oldSwapchain = oldSwapchain};

    VkSwapchainKHR swapchain{};
    VkResult res{vkCreateSwapchainKHR(device.logical, &swapchainCI, nullptr,
                                      &swapchain)};

    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan swapchain");
        assert(false);
    }

    uint32_t imageCount{};
    vkGetSwapchainImagesKHR(device.logical, swapchain, &imageCount, nullptr);
    std::vector<VkImage> images(imageCount);
    vkGetSwapchainImagesKHR(device.logical, swapchain, &imageCount,
                            images.data());

    VkImageViewCreateInfo imageViewCI{
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = swapchainCI.imageFormat,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        }};

    std::vector<VkImageView> imageViews(imageCount);
    for (size_t i{}; i < imageViews.size(); i++) {
        imageViewCI.image = images[i];
        res = vkCreateImageView(device.logical, &imageViewCI, nullptr,
                                &imageViews[i]);
        if (res != VK_SUCCESS) {
            spdlog::error("Could not create swapchain image views");
        }
    }

    return Swapchain{
        .handle = swapchain,
        .images = images,
        .imageViews = imageViews,
        .format = swapchainCI.imageFormat,
        .extent = swapchainCI.imageExtent,
    };
}

void SYN::VK::destroySwapchain(Swapchain &swapchain, const Device &device) {
    for (const auto &imageView : swapchain.imageViews) {
        vkDestroyImageView(device.logical, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device.logical, swapchain.handle, nullptr);
    swapchain = {};
}

namespace {
VkSurfaceFormatKHR findBestSurfaceFormat(const Device &device,
                                         VkSurfaceKHR surface) {
    uint32_t surfaceFormatsCount{};
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.physical, surface,
                                         &surfaceFormatsCount, nullptr);
    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device.physical, surface, &surfaceFormatsCount, surfaceFormats.data());

    VkSurfaceFormatKHR selectedSurfaceFormat{};
    bool idealSurfaceFormatFound{};
    for (const auto &surfaceFormat : surfaceFormats) {
        if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            selectedSurfaceFormat = surfaceFormat;
            idealSurfaceFormatFound = true;
            break;
        }
        selectedSurfaceFormat = surfaceFormat;
    }

    if (!idealSurfaceFormatFound) {
        spdlog::warn("Could not find ideal surface format for swapchain");
    }

    return selectedSurfaceFormat;
}
VkPresentModeKHR findBestPresentMode(const Device &device,
                                     VkSurfaceKHR surface) {
    uint32_t presentModesCount{};
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.physical, surface,
                                              &presentModesCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModesCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device.physical, surface, &presentModesCount, presentModes.data());

    VkPresentModeKHR selectedPresentMode{
        VK_PRESENT_MODE_FIFO_KHR}; // guarenteed to be available
    bool idealPresentModeFound{};
    for (const auto &presentMode : presentModes) {
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            selectedPresentMode = presentMode;
            idealPresentModeFound = true;
            break;
        }
    }

    if (!idealPresentModeFound) {
        spdlog::warn("Could not find ideal present mode for swapchain");
    }

    return selectedPresentMode;
}
VkExtent2D
calcSurfaceExtent(const VkSurfaceCapabilitiesKHR &surfaceCapabilities,
                  SYN::Window &window) {
    VkExtent2D surfaceExtent{};
    if (surfaceCapabilities.currentExtent.width == UINT32_MAX &&
        surfaceCapabilities.currentExtent.height == UINT32_MAX) {
        int width{};
        int height{};
        glfwGetFramebufferSize(window.getHandle(), &width, &height);

        surfaceExtent.width =
            std::clamp(static_cast<uint32_t>(std::abs(width)),
                       surfaceCapabilities.minImageExtent.width,
                       surfaceCapabilities.maxImageExtent.width);

        surfaceExtent.height =
            std::clamp(static_cast<uint32_t>(std::abs(height)),
                       surfaceCapabilities.minImageExtent.height,
                       surfaceCapabilities.maxImageExtent.height);
    } else {
        surfaceExtent = surfaceCapabilities.currentExtent;
    }

    return surfaceExtent;
}
} // namespace
