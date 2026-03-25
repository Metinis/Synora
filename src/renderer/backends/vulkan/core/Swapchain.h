#pragma once

#include "Device.h"
#include "Image.h"

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN {
class Window;
}
namespace SYN::VK {

struct Swapchain {
    VkSwapchainKHR handle;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    VkFormat format;
    VkExtent2D extent;
};

Swapchain createSwapchain(const Device &device, VkSurfaceKHR surface,
                          Window &window,
                          VkSwapchainKHR oldSwapchain = VK_NULL_HANDLE);

void destroySwapchain(Swapchain &swapchain, const Device &device);
} // namespace SYN::VK
