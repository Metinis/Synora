#include "Backend.h"
#include "core/Buffer.h"
#include "core/Commands.h"
#include "core/DebugMessenger.h"
#include "core/Device.h"
#include "core/Image.h"
#include "core/Instance.h"
#include "core/Pipeline.h"
#include "core/StagingBuffer.h"
#include "core/Swapchain.h"
#include "renderer/RenderTypes.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <vk_mem_alloc.h>

#include <PuzzleEngine/core/Window.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

namespace {
constexpr uint32_t c_MB{1024 * 1024};

} // namespace

void SYN::VK::VulkanBackend::init(Window *window) {
    initContext(window);

    initDescriptorSetLayout();
    initPipelineLayout();
    initDescriptorSets();

    initFrameData(m_Swapchain);

    m_StagingBuffer =
        StagingBuffer().create(m_Device, m_Allocator, c_MB * 64 * 8);

    VkSamplerCreateInfo samplerCI{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy =
            std::min(4.f, m_Device.properties.limits.maxSamplerAnisotropy),
        .maxLod = VK_LOD_CLAMP_NONE};

    vkCreateSampler(m_Device.logical, &samplerCI, nullptr, &m_DefaultSampler);

    m_SwapchainAttachmentHandle = m_Attachments.insert(Attachment{
        .size = AttachmentSize::fixed, // technically its relative, but relative
                                       // triggers recreation when the swapchain
                                       // goes out of date, when the images
                                       // arent created but instead acquired
                                       // from the swapchain. Hence we use fixed
        .isSampleable = false,         // tis an output
    });
}

void SYN::VK::VulkanBackend::shutdown() {
    vkDeviceWaitIdle(m_Device.logical);

    vkDestroySampler(m_Device.logical, m_DefaultSampler, nullptr);

    for (const auto &[handle, attachment] : m_Attachments) {
        if (handle == m_SwapchainAttachmentHandle) {
            continue;
        }
        for (auto image : attachment.images) {
            spdlog::warn("Texture {} may have been leaked", handle.id);
            destroyImage(m_Device, m_Allocator, image);
        }
    }
    m_Attachments.clear();

    for (auto [handle, texture] : m_Textures) {
        spdlog::warn("Texture {} may have been leaked", handle.id);
        destroyImage(m_Device, m_Allocator, texture.image);
    }
    m_Textures.clear();

    for (auto [handle, buffer] : m_Buffers) {
        spdlog::warn("Buffer {} may have been leaked", handle.id);
        VK::destroyBuffer(m_Allocator, buffer);
    }
    m_Buffers.clear();

    for (auto [handle, pipeline] : m_Pipelines) {
        spdlog::warn("Pipeline {} may have been leaked", handle.id);
        vkDestroyPipeline(m_Device.logical, pipeline, nullptr);
    }

    for (auto &frame : m_FrameData) {
        frame.UBOBuffer.destroy(m_Allocator);
        vkDestroyFence(m_Device.logical, frame.renderFinishedFence, nullptr);
        vkDestroySemaphore(m_Device.logical, frame.imageAvailableSemaphore,
                           nullptr);
        vkDestroyCommandPool(m_Device.logical, frame.graphicsCmdPool, nullptr);
        for (auto &buffer : frame.buffersToFree) {
            VK::destroyBuffer(m_Allocator, buffer);
        }
        for (auto &image : frame.imagesToFree) {
            destroyImage(m_Device, m_Allocator, image);
        }
        for (auto &pipeline : frame.pipelinesToFree) {
            vkDestroyPipeline(m_Device.logical, pipeline, nullptr);
        }
    }

    for (const auto &semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device.logical, semaphore, nullptr);
    }

    vkDestroyDescriptorSetLayout(m_Device.logical,
                                 m_BindlessDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Device.logical, m_UBODescriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorPool(m_Device.logical, m_DescriptorPool, nullptr);

    vkDestroyPipelineLayout(m_Device.logical, m_GraphicsPipelineLayout,
                            nullptr);

    vkDestroyCommandPool(m_Device.logical, m_TransientCmdPool, nullptr);
    destroySwapchain(m_Swapchain, m_Device);
    m_StagingBuffer.destroy(m_Device, m_Allocator);
    vmaDestroyAllocator(m_Allocator);
    destroyDevice(m_Device);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    destroyDebugMessenger(m_Instance, m_DebugUtilsMessenger);
    vkDestroyInstance(m_Instance, nullptr);
}

void SYN::VK::VulkanBackend::initContext(Window *window) {
    m_Instance = createInstance();

    m_DebugUtilsMessenger = createDebugMessenger(m_Instance);

    VkResult res{glfwCreateWindowSurface(m_Instance, window->getHandle(),
                                         nullptr, &m_Surface)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan surface");
    }

    m_Device = createDevice(m_Instance, m_Surface);

    VmaAllocatorCreateInfo allocatorCreateInfo = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = m_Device.physical,
        .device = m_Device.logical,
        .instance = m_Instance,
        .vulkanApiVersion = VK_API_VERSION_1_3,
    };

    res = vmaCreateAllocator(&allocatorCreateInfo, &m_Allocator);
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan allocator");
    }

    m_Swapchain = createSwapchain(m_Device, m_Surface, *window);

    VkCommandPoolCreateInfo transientCmdPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex =
            m_Device.queues.at(QueueFamily::graphics).familyIndex};

    vkCreateCommandPool(m_Device.logical, &transientCmdPoolCI, nullptr,
                        &m_TransientCmdPool);
}

void SYN::VK::VulkanBackend::initDescriptorSetLayout() {
    uint32_t textureDescriptorCount{
        std::min(c_MaxBindlessTextures,
                 m_Device.properties.limits.maxDescriptorSetSampledImages)};

    VkDescriptorBindingFlags textureBindingFlags{
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};
    VkDescriptorBindingFlags uboBindingFlags{
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

    VkDescriptorSetLayoutBindingFlagsCreateInfo textureBindingFlagsCI{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &textureBindingFlags,
    };
    VkDescriptorSetLayoutBindingFlagsCreateInfo uboBindingFlagsCI{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &uboBindingFlags,
    };

    VkDescriptorSetLayoutBinding bindlessTextureBinding{
        .binding = c_TextureBinding,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = textureDescriptorCount,
        .stageFlags = VK_SHADER_STAGE_ALL};

    VkDescriptorSetLayoutBinding uboBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL};

    VkDescriptorSetLayoutCreateInfo bindlessLayoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &textureBindingFlagsCI,
        .bindingCount = 1,
        .pBindings = &bindlessTextureBinding,
    };
    VkDescriptorSetLayoutCreateInfo uboLayoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &uboBindingFlagsCI,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = 1,
        .pBindings = &uboBinding,
    };

    VkResult res{vkCreateDescriptorSetLayout(m_Device.logical,
                                             &bindlessLayoutCI, nullptr,
                                             &m_BindlessDescriptorSetLayout)};

    if (res != VK_SUCCESS) {
        spdlog::error("Could not create descriptor set layout. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }

    res = vkCreateDescriptorSetLayout(m_Device.logical, &uboLayoutCI, nullptr,
                                      &m_UBODescriptorSetLayout);

    if (res != VK_SUCCESS) {
        spdlog::error("Could not create descriptor set layout. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }

    m_BindlessTextureIndexFreelist.reserve(c_MaxBindlessTextures);
    for (size_t i{}; i < c_MaxBindlessTextures; i++) {
        m_BindlessTextureIndexFreelist.emplace_back(i);
    }
}

void SYN::VK::VulkanBackend::initPipelineLayout() {
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_ALL,
        .size = c_MinGuarenteedPushConstantSize,
    };
    std::array<VkDescriptorSetLayout, 2> layouts{m_BindlessDescriptorSetLayout,
                                                 m_UBODescriptorSetLayout};
    VkPipelineLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = layouts.size(),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };

    VkResult res{vkCreatePipelineLayout(m_Device.logical, &layoutCI, nullptr,
                                        &m_GraphicsPipelineLayout)};
    if (res != VK_SUCCESS) {
        spdlog::error(
            "Could not create graphics pipeline layout. VkResult = {}",
            static_cast<int>(res));
        assert(false);
    }
}

void SYN::VK::VulkanBackend::initDescriptorSets() {
    uint32_t textureDescriptorCount{
        std::min(c_MaxBindlessTextures,
                 m_Device.properties.limits.maxDescriptorSetSampledImages)};

    std::array<VkDescriptorPoolSize, 2> poolSizes{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = textureDescriptorCount,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            .descriptorCount = 1,
        },
    };

    VkDescriptorPoolCreateInfo bindlessDescriptorPoolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
        .maxSets = 2,
        .poolSizeCount = poolSizes.size(),
        .pPoolSizes = poolSizes.data(),
    };
    VkResult res{vkCreateDescriptorPool(m_Device.logical,
                                        &bindlessDescriptorPoolCI, nullptr,
                                        &m_DescriptorPool)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create descriptor pool. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }

    std::array<VkDescriptorSetLayout, 2> layouts{m_BindlessDescriptorSetLayout,
                                                 m_UBODescriptorSetLayout};
    VkDescriptorSetAllocateInfo descriptorSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_DescriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
    };

    std::array<VkDescriptorSet, 2> descriptorSets{};
    res = vkAllocateDescriptorSets(m_Device.logical, &descriptorSetAllocInfo,
                                   descriptorSets.data());
    m_BindlessDescriptorSet = descriptorSets[0];
    m_UBODescriptorSet = descriptorSets[1];

    if (res != VK_SUCCESS) {
        spdlog::error("Could not allocate descriptor sets. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }
}

void SYN::VK::VulkanBackend::initFrameData(const Swapchain &swapchain) {
    for (size_t i{}; i < c_MaxFramesInFlight; i++) {
        VkCommandPoolCreateInfo graphicsCmdPoolCI{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex =
                m_Device.queues[QueueFamily::graphics].familyIndex};
        VkCommandPool graphicsCmdPool{};
        vkCreateCommandPool(m_Device.logical, &graphicsCmdPoolCI, nullptr,
                            &graphicsCmdPool);

        VkCommandBufferAllocateInfo cmdBufferAllocInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = graphicsCmdPool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VkCommandBuffer graphicsCmdBuffer{};
        VkResult res{vkAllocateCommandBuffers(
            m_Device.logical, &cmdBufferAllocInfo, &graphicsCmdBuffer)};
        if (res != VK_SUCCESS) {
            spdlog::error("Could not allocate command buffers, VkResult = {}",
                          static_cast<int>(res));
        }

        VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                  .flags = VK_FENCE_CREATE_SIGNALED_BIT};
        VkFence renderFinishedFence{};
        vkCreateFence(m_Device.logical, &fenceCI, nullptr,
                      &renderFinishedFence);

        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkSemaphore imageAvailableSemaphore{};
        vkCreateSemaphore(m_Device.logical, &semaphoreCI, nullptr,
                          &imageAvailableSemaphore);

        DynamicUBO uboBuffer{};
        uboBuffer.create(m_Device, m_Allocator, 1 * c_MB);

        m_FrameData[i] =
            FrameData{.graphicsCmdPool = graphicsCmdPool,
                      .graphicsCmdBuffer = graphicsCmdBuffer,
                      .renderFinishedFence = renderFinishedFence,
                      .imageAvailableSemaphore = imageAvailableSemaphore,
                      .UBOBuffer = std::move(uboBuffer)};
    }
    for (const auto &_ : swapchain.images) {
        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkSemaphore renderFinishedSemaphore{};
        vkCreateSemaphore(m_Device.logical, &semaphoreCI, nullptr,
                          &renderFinishedSemaphore);
        m_RenderFinishedSemaphores.emplace_back(renderFinishedSemaphore);
    }
}

void SYN::VK::VulkanBackend::recreateSwapchain(Window &window) {
    vkDeviceWaitIdle(m_Device.logical);

    Swapchain newSwapchain{
        createSwapchain(m_Device, m_Surface, window, m_Swapchain.handle)};
    destroySwapchain(m_Swapchain, m_Device);
    m_Swapchain = newSwapchain;

    for (const auto &[handle, attachment] : m_Attachments) {
        if (handle == m_SwapchainAttachmentHandle) {
            continue;
        }
        if (attachment.size != AttachmentSize::relative) {
            continue;
        }

        for (size_t i{}; i < c_MaxFramesInFlight; i++) {
            Image &image{attachment.images[i]};

            VkFormat format{image.format};
            VkImageUsageFlags usage{image.usage};
            VkImageAspectFlags aspect{image.subresourceRange.aspectMask};
            destroyImage(m_Device, m_Allocator, image);
            image = createImage(m_Device, m_Allocator, format,
                                m_Swapchain.extent, usage, aspect);

            if (!attachment.isSampleable) {
                continue;
            }

            uint32_t descriptorElement{attachment.bindlessSamplerIndices[i]};

            VkDescriptorImageInfo imageInfo{
                .sampler = m_DefaultSampler,
                .imageView = image.view,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            };

            VkWriteDescriptorSet bindlessWrite{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_BindlessDescriptorSet,
                .dstBinding = 0,
                .dstArrayElement = descriptorElement,
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .pImageInfo = &imageInfo,
            };

            vkUpdateDescriptorSets(m_Device.logical, 1, &bindlessWrite, 0,
                                   nullptr);
        }
    }
}
