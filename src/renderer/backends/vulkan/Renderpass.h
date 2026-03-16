#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

namespace SYN::VK {
struct Image;

struct alignas(16) Vertex {
    glm::vec3 pos;
};

struct PushConstants {
    VkDeviceAddress vertexBuffer;
};

void cmdDefaultRenderPass(VkCommandBuffer cmdBuffer, VkPipeline pipeline,
                          VkPipelineLayout layout,
                          const PushConstants &pushConstants,
                          uint32_t vertexCount, Image &renderTarget);
} // namespace SYN::VK
