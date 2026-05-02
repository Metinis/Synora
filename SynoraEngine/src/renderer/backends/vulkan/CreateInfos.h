#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace SYN::VK::CI {
constexpr VkSamplerCreateInfo c_LinearClampCI{
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .anisotropyEnable = VK_TRUE,
    .maxLod = VK_LOD_CLAMP_NONE};

constexpr VkSamplerCreateInfo c_NearestClampCI{
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_NEAREST,
    .minFilter = VK_FILTER_NEAREST,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    .anisotropyEnable = VK_FALSE,
    .maxLod = VK_LOD_CLAMP_NONE};
constexpr VkSamplerCreateInfo c_LinearRepeatCI{
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_LINEAR,
    .minFilter = VK_FILTER_LINEAR,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable = VK_TRUE,
    .maxLod = VK_LOD_CLAMP_NONE};

constexpr VkSamplerCreateInfo c_NearestRepeatCI{
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .magFilter = VK_FILTER_NEAREST,
    .minFilter = VK_FILTER_NEAREST,
    .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
    .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
    .anisotropyEnable = VK_FALSE,
    .maxLod = VK_LOD_CLAMP_NONE};

// the index of these should also mirror their indices in the samplers
// descriptor set
constexpr std::array<VkSamplerCreateInfo, 4> c_SamplerCIs{
    c_LinearClampCI,
    c_NearestClampCI,
    c_LinearRepeatCI,
    c_NearestRepeatCI,
};

} // namespace SYN::VK::CI
