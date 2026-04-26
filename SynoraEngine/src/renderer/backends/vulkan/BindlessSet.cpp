#include "BindlessSet.h"
#include "Limits.h"
#include "core/Device.h"
#include <spdlog/spdlog.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

namespace {
VkDescriptorSetLayout createLayout(const Device &device,
                                   uint32_t textureDescriptorCount,
                                   uint32_t samplerDescriptorCount);
VkDescriptorPool createPool(const Device &device,
                            uint32_t textureDescriptorCount,
                            uint32_t samplerDescriptorCount);
VkDescriptorSet createSet(const Device &device, VkDescriptorPool pool,
                          VkDescriptorSetLayout layout);
} // namespace

SYN::VK::BindlessSet::~BindlessSet() {
    if (m_DescriptorPool != VK_NULL_HANDLE) {
        spdlog::warn("Bindless descriptor set leaked");
        assert(false);
    }
}

void SYN::VK::BindlessSet::create(const Device &device) {
    uint32_t textureDescriptorCount{
        std::min(Limits::c_MaxBindlessTextures,
                 device.properties.limits.maxDescriptorSetSampledImages)};
    uint32_t samplerDescriptorCount{
        std::min(Limits::c_SamplerCount,
                 device.properties.limits.maxDescriptorSetSamplers)};

    m_DescriptorSetLayout =
        createLayout(device, textureDescriptorCount, samplerDescriptorCount);
    m_DescriptorPool =
        createPool(device, textureDescriptorCount, samplerDescriptorCount);
    m_DescriptorSet =
        createSet(device, m_DescriptorPool, m_DescriptorSetLayout);
}

void SYN::VK::BindlessSet::destroy(const Device &device) {
    vkDestroyDescriptorPool(device.logical, m_DescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(device.logical, m_DescriptorSetLayout,
                                 nullptr);

    *this = {};
}

namespace {

VkDescriptorSetLayout createLayout(const Device &device,
                                   uint32_t textureDescriptorCount,
                                   uint32_t samplerDescriptorCount) {

    std::array<VkDescriptorBindingFlags, 2> bindingFlags{
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindlessBindingFlagsCI{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = bindingFlags.size(),
        .pBindingFlags = bindingFlags.data(),
    };

    VkDescriptorSetLayoutBinding textureBinding{
        .binding = Limits::c_TextureBinding,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .descriptorCount = textureDescriptorCount,
        .stageFlags = VK_SHADER_STAGE_ALL,
    };

    VkDescriptorSetLayoutBinding samplerBinding{
        .binding = Limits::c_SamplerBinding,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .descriptorCount = samplerDescriptorCount,
        .stageFlags = VK_SHADER_STAGE_ALL,
    };

    std::array<VkDescriptorSetLayoutBinding, 2> bindings{textureBinding,
                                                         samplerBinding};

    VkDescriptorSetLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &bindlessBindingFlagsCI,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = bindings.size(),
        .pBindings = bindings.data(),
    };

    VkDescriptorSetLayout layout{};
    VkResult res{vkCreateDescriptorSetLayout(device.logical, &layoutCI, nullptr,
                                             &layout)};

    if (res != VK_SUCCESS) {
        spdlog::error("Could not create bindless descriptor set layout. "
                      "VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }

    return layout;
}

VkDescriptorPool createPool(const Device &device,
                            uint32_t textureDescriptorCount,
                            uint32_t samplerDescriptorCount) {

    std::array<VkDescriptorPoolSize, 2> poolSizes{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = textureDescriptorCount,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = samplerDescriptorCount,
        },
    };

    VkDescriptorPoolCreateInfo bindlessDescriptorPoolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 1,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };

    VkDescriptorPool pool{};
    VkResult res{vkCreateDescriptorPool(
        device.logical, &bindlessDescriptorPoolCI, nullptr, &pool)};
    if (res != VK_SUCCESS) {
        spdlog::error(
            "Could not create bindless descriptor pool. VkResult = {}",
            static_cast<int>(res));
        assert(false);
    }

    return pool;
}

VkDescriptorSet createSet(const Device &device, VkDescriptorPool pool,
                          VkDescriptorSetLayout layout) {
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = pool,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };

    VkDescriptorSet set{};
    VkResult res{vkAllocateDescriptorSets(device.logical,
                                          &descriptorSetAllocInfo, &set)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not allocate descriptor sets. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }

    return set;
}
} // namespace
