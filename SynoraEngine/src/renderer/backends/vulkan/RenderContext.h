#pragma once

#include "Limits.h"
#include "core/Device.h"
#include <vulkan/vulkan.h>

struct VmaAllocator_T;

namespace SYN::VK {
struct RenderContext {
    VkInstance instance{};
    VkSurfaceKHR surface{};
    VkDebugUtilsMessengerEXT debugUtilsMessenger{};
    VK::Device device{};

    VmaAllocator_T *allocator{};
    VkCommandPool transientCmdPool{};

    VkDescriptorPool descriptorPool{};

    VkDescriptorSetLayout bindlessDescriptorSetLayout{};
    VkDescriptorSetLayout uboDescriptorSetLayout{};

    VkPipelineLayout bindlessPipelineLayout;

    VkDescriptorSet bindlessDescriptorSet{};
    VkDescriptorSet uboDescriptorSet{};

    std::array<VkSampler, VK::Limits::c_SamplerCount> samplers{};

    // imgui
    VkDescriptorPool imGUIDescriptorPool{};
};

RenderContext createRenderContext(void);
} // namespace SYN::VK
