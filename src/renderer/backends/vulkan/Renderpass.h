#pragma once

#include <vulkan/vulkan.h>

namespace SYN::VK {
struct Image;

void cmdDefaultRenderPass(VkCommandBuffer cmdBuffer, VkPipeline pipeline,
                          Image &renderTarget);
} // namespace SYN::VK
