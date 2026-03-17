#pragma once

#include <vulkan/vulkan.h>

namespace SYN::VK {
struct Image {
    VkImage handle;
    VkImageView view;
    VkExtent2D extent;
    VkImageSubresourceRange subresourceRange;
};
} // namespace SYN::VK
