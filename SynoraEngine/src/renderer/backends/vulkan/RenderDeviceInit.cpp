#include "Limits.h"
#include "RenderDevice.h"
#include "core/Buffer.h"
#include "core/Commands.h"
#include "core/DebugMessenger.h"
#include "core/Device.h"
#include "core/Image.h"
#include "core/Instance.h"
#include "core/Pipeline.h"
#include "core/StagingBuffer.h"
#include "core/Swapchain.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <vk_mem_alloc.h>

#include <SynoraEngine/core/Window.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan_core.h>

#include "imgui_impl_glfw.h"
#include "imgui_internal.h"
#include "renderer/backends/vulkan/CreateInfos.h"

using namespace SYN;
using namespace SYN::VK;

namespace {
constexpr uint32_t c_MB{1024 * 1024};

} // namespace

void SYN::RenderDevice::init(Window &window) {
    initContext(&window);
    {
        VkSemaphoreTypeCreateInfo semaphoreTypeCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
            .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
            .initialValue = 0,
        };
        VkSemaphoreCreateInfo timelineSemaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
            .pNext = &semaphoreTypeCI,
        };
        vkCreateSemaphore(m_Device.logical, &timelineSemaphoreCI, nullptr,
                          &m_GPUSubmissionSemaphore);
        vkCreateSemaphore(m_Device.logical, &timelineSemaphoreCI, nullptr,
                          &m_CPUSubmissionSemaphore);
    }

    initFrameData(m_Swapchain);
    initDescriptorSetLayout();
    initDescriptorSets();
    initPipelineLayout();

    initSamplers();

    initImGUI(&window);

    m_StagingBuffer =
        StagingBuffer().create(m_Device, m_Allocator, c_MB * 64 * 4);

    uint32_t descriptorCount{
        std::min(Limits::c_MaxBindlessTextures,
                 m_Device.properties.limits.maxDescriptorSetSampledImages)};

    m_BindlessTextureIndexFreelist.reserve(descriptorCount);
    for (uint32_t i = 0; i < descriptorCount; i++) {
        m_BindlessTextureIndexFreelist.emplace_back(i);
    }
}

void SYN::RenderDevice::shutdown() {
    vkDeviceWaitIdle(m_Device.logical);

    for (auto handle : m_SwapchainAttachmentHandles) {
        m_Attachments.remove(handle);
    }

    for (auto &frame : m_FrameData) {
        frame.UBOBuffer.destroy(m_Allocator);

        vkDestroyFence(m_Device.logical, frame.renderFinishedFence, nullptr);
        vkDestroySemaphore(m_Device.logical, frame.imageAvailableSemaphore,
                           nullptr);
        vkDestroyCommandPool(m_Device.logical, frame.graphicsCmdPool, nullptr);
    }

    for (const auto &semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device.logical, semaphore, nullptr);
    }
    vkDestroySemaphore(m_Device.logical, m_GPUSubmissionSemaphore, nullptr);
    vkDestroySemaphore(m_Device.logical, m_CPUSubmissionSemaphore, nullptr);

    for (auto sampler : m_Samplers) {
        vkDestroySampler(m_Device.logical, sampler, nullptr);
    }

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
    m_Pipelines.clear();

    for (auto [handle, attachment] : m_Attachments) {
        destroyImage(m_Device, m_Allocator, attachment.image);
    }
    m_Attachments.clear();

    vkDestroyDescriptorSetLayout(m_Device.logical,
                                 m_BindlessDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Device.logical, m_UBODescriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorPool(m_Device.logical, m_DescriptorPool, nullptr);

    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(m_Device.logical, m_ImGUIDescriptorPool, nullptr);

    vkDestroyPipelineLayout(m_Device.logical, m_BindlessPipelineLayout,
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

void SYN::RenderDevice::initContext(Window *window) {
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
    m_SwapchainAttachmentHandles.resize(m_Swapchain.images.size());

    for (size_t i{}; i < m_Swapchain.images.size(); i++) {
        Image swapchainImage{
            .handle = m_Swapchain.images[i],
            .view = m_Swapchain.imageViews[i],
            .extent = m_Swapchain.extent,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
        };

        // TODO: add swapchain attachment to bindless descriptors and abstract
        // bindless descriptor system into its own thingy
        Attachment swapchainAttachment{
            .image = swapchainImage,
            .size = AttachmentSize::fixed,
        };
        AttachmentHandle handle{m_Attachments.insert(swapchainAttachment)};
        m_SwapchainAttachmentHandles[i] = handle;
    }

    VkCommandPoolCreateInfo transientCmdPoolCI{
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex =
            m_Device.queues.at(QueueFamily::graphics).familyIndex};

    vkCreateCommandPool(m_Device.logical, &transientCmdPoolCI, nullptr,
                        &m_TransientCmdPool);
}

void SYN::RenderDevice::initDescriptorSetLayout() {
    uint32_t textureDescriptorCount{
        std::min(Limits::c_MaxBindlessTextures,
                 m_Device.properties.limits.maxDescriptorSetSampledImages)};
    uint32_t samplerDescriptorCount{
        std::min(Limits::c_SamplerCount,
                 m_Device.properties.limits.maxDescriptorSetSamplers)};
    uint32_t storageImageDescriptorCount{
        std::min(Limits::c_MaxStorageImages,
                 m_Device.properties.limits.maxDescriptorSetStorageImages)};

    VkDescriptorBindingFlags uboFlags{
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

    std::array<VkDescriptorBindingFlags, 3> bindlessFlags{
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT,
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindlessBindingFlagsCI{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = bindlessFlags.size(),
        .pBindingFlags = bindlessFlags.data(),
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo uboBindingFlagsCI{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &uboFlags,
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
    VkDescriptorSetLayoutBinding storageImageBinding{
        .binding = Limits::c_StorageImageBinding,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .descriptorCount = storageImageDescriptorCount,
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
    };

    std::array<VkDescriptorSetLayoutBinding, 3> bindlessBindings{
        textureBinding, samplerBinding, storageImageBinding};

    VkDescriptorSetLayoutBinding uboBinding{
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_ALL};

    VkDescriptorSetLayoutCreateInfo bindlessLayoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &bindlessBindingFlagsCI,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = bindlessBindings.size(),
        .pBindings = bindlessBindings.data(),
    };

    VkDescriptorSetLayoutCreateInfo uboLayoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &uboBindingFlagsCI,
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
}

void SYN::RenderDevice::initPipelineLayout() {
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_ALL,
        .size = Limits::c_MinGuarenteedPushConstantSize,
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
                                        &m_BindlessPipelineLayout)};
    if (res != VK_SUCCESS) {
        spdlog::error(
            "Could not create graphics pipeline layout. VkResult = {}",
            static_cast<int>(res));
        assert(false);
    }
}

void SYN::RenderDevice::initDescriptorSets() {
    uint32_t textureDescriptorCount{
        std::min(Limits::c_MaxBindlessTextures,
                 m_Device.properties.limits.maxDescriptorSetSampledImages)};
    uint32_t samplerDescriptorCount{
        std::min(Limits::c_SamplerCount,
                 m_Device.properties.limits.maxDescriptorSetSamplers)};
    uint32_t storageImageDescriptorCount{
        std::min(Limits::c_MaxStorageImages,
                 m_Device.properties.limits.maxDescriptorSetStorageImages)};

    std::array<VkDescriptorPoolSize, 4> poolSizes{
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .descriptorCount = textureDescriptorCount,
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_SAMPLER,
            .descriptorCount = static_cast<uint32_t>(samplerDescriptorCount),
        },
        VkDescriptorPoolSize{
            .type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .descriptorCount =
                static_cast<uint32_t>(storageImageDescriptorCount),
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

void SYN::RenderDevice::initImGUI(Window *window) {
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = (uint32_t)std::size(poolSizes);
    poolInfo.pPoolSizes = poolSizes;

    VkResult res = (vkCreateDescriptorPool(m_Device.logical, &poolInfo, nullptr,
                                           &m_ImGUIDescriptorPool));

    if (res != VK_SUCCESS) {
        spdlog::error("Could not create descriptor pool. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }

    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForVulkan(window->getHandle(), true);

    ImGui_ImplVulkan_InitInfo initInfo = {};
    initInfo.Instance = m_Instance;
    initInfo.PhysicalDevice = m_Device.physical;
    initInfo.Device = m_Device.logical;
    initInfo.Queue = m_Device.queues[QueueFamily::graphics].handle;
    initInfo.DescriptorPool = m_ImGUIDescriptorPool;
    initInfo.MinImageCount = 3;
    initInfo.ImageCount = 3;
    initInfo.UseDynamicRendering = true;

    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount =
        1;
    initInfo.PipelineInfoMain.PipelineRenderingCreateInfo
        .pColorAttachmentFormats = &m_Swapchain.format;
    initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&initInfo);
}

void SYN::RenderDevice::initFrameData(const Swapchain &swapchain) {
    for (size_t i{}; i < Limits::c_MaxFramesInFlight; i++) {
        VkCommandPoolCreateInfo graphicsCmdPoolCI{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex =
                m_Device.queues.at(QueueFamily::graphics).familyIndex};
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

void SYN::RenderDevice::recreateSwapchain(Window &window) {
    vkDeviceWaitIdle(m_Device.logical);

    Swapchain newSwapchain{
        createSwapchain(m_Device, m_Surface, window, m_Swapchain.handle)};
    destroySwapchain(m_Swapchain, m_Device);
    m_Swapchain = newSwapchain;

    for (auto handle : m_SwapchainAttachmentHandles) {
        m_Attachments.remove(handle);
    }

    m_SwapchainAttachmentHandles.clear();
    m_SwapchainAttachmentHandles.resize(m_Swapchain.images.size());

    for (size_t i{}; i < m_Swapchain.images.size(); i++) {
        Image swapchainImage{
            .handle = m_Swapchain.images[i],
            .view = m_Swapchain.imageViews[i],
            .extent = m_Swapchain.extent,
            .subresourceRange =
                VkImageSubresourceRange{
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .levelCount = 1,
                    .layerCount = 1,
                },
        };

        Attachment swapchainAttachment{
            .image = swapchainImage,
            .size = AttachmentSize::fixed,
        };
        AttachmentHandle handle{m_Attachments.insert(swapchainAttachment)};
        m_SwapchainAttachmentHandles[i] = handle;
    }

    for (auto [handle, attachment] : m_Attachments) {
        if (attachment.size != AttachmentSize::relative) {
            continue;
        }

        {
            Image prevImage{attachment.image};
            m_Attachments[handle].image = createImage(
                m_Device, m_Allocator, prevImage.format, m_Swapchain.extent,
                prevImage.usage, prevImage.subresourceRange.aspectMask,
                prevImage.samples, prevImage.mipLevels, prevImage.layerCount,
                prevImage.isCubeMap);

            destroyImage(m_Device, m_Allocator, prevImage);
        }

        generateMipChain(m_TransientCmdPool, m_Device, attachment.image,
                         VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                         VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                         VK_ACCESS_2_SHADER_READ_BIT);

        std::vector<VkDescriptorImageInfo> imageInfos;
        std::vector<VkWriteDescriptorSet> bindlessWrites;
        // TODO: transition images
        VkDescriptorImageInfo imageInfo{
            .imageView = m_Attachments[handle].image.view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        };
        imageInfos.emplace_back(imageInfo);

        VkWriteDescriptorSet textureWrite{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_BindlessDescriptorSet,
            .dstBinding = Limits::c_TextureBinding,
            .dstArrayElement = attachment.bindlessTextureIndex,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &imageInfos.back(),
        };
        bindlessWrites.emplace_back(textureWrite);
        if (attachment.bindlessStorageImageIndex.has_value()) {
            VkWriteDescriptorSet storageImageWrite{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_BindlessDescriptorSet,
                .dstBinding = Limits::c_StorageImageBinding,
                .dstArrayElement = attachment.bindlessStorageImageIndex.value(),
                .descriptorCount = 1,
                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                .pImageInfo = &imageInfos.back(),
            };
            bindlessWrites.emplace_back(storageImageWrite);
        }

        vkUpdateDescriptorSets(m_Device.logical, bindlessWrites.size(),
                               bindlessWrites.data(), 0, nullptr);
    }
}

void SYN::RenderDevice::initSamplers() {
    for (uint32_t i{}; i < Limits::c_SamplerCount; i++) {
        VkSamplerCreateInfo samplerCI{CI::c_SamplerCIs.at(i)};
        samplerCI.maxAnisotropy =
            std::min(4.f, m_Device.properties.limits.maxSamplerAnisotropy);

        vkCreateSampler(m_Device.logical, &samplerCI, nullptr, &m_Samplers[i]);

        VkDescriptorImageInfo imageInfo{.sampler = m_Samplers[i]};
        VkWriteDescriptorSet bindlessWrite{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_BindlessDescriptorSet,
            .dstBinding = Limits::c_SamplerBinding,
            .dstArrayElement = i,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
            .pImageInfo = &imageInfo};

        vkUpdateDescriptorSets(m_Device.logical, 1, &bindlessWrite, 0, nullptr);
    }
}
