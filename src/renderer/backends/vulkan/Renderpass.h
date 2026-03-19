#pragma once

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

namespace SYN::VK {
struct Image;

struct alignas(16) Vertex {
    glm::vec3 pos;
    float u;
    alignas(16) float v;
};

struct PushConstants {
    VkDeviceAddress vertexBuffer;
};

void defaultRenderPassCmd(VkCommandBuffer cmdBuffer, VkPipeline pipeline,
                          VkPipelineLayout layout,
                          const PushConstants &pushConstants,
                          VkDescriptorSet bindlessDescriptorSet,
                          uint32_t vertexCount, const Image &renderTarget,
                          const Image &depthTarget,
                          VkImageLayout renderTargetLayout);
} // namespace SYN::VK
