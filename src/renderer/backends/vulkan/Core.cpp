#include "Core.h"
#include "Buffer.h"
#include "Commands.h"
#include "DebugMessenger.h"
#include "Instance.h"
#include "Pipeline.h"
#include "Renderpass.h"
#include "StagingBuffer.h"
#include "Swapchain.h"
#include "renderer/backends/vulkan/Image.h"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstring>
#include <glm/glm.hpp>
#include <iterator>
#include <ranges>
#include <spdlog/spdlog.h>
#include <stb_image.h>
#include <vk_mem_alloc.h>

#include <PuzzleEngine/core/Window.h>
#include <vulkan/vulkan_core.h>

#include <queue>

using namespace SYN;
using namespace SYN::VK;

namespace {
constexpr uint32_t c_MB{1024 * 1024};

VkShaderModule createShaderModule(const Device &device,
                                  const std::string &path);

// TODO: change this, its a temp until asset manager is made

} // namespace

void VulkanBackend::init(SYN::Window &window) {
    initContext(window);

    initDescriptorSetLayout();
    initPipelineLayout();
    initDescriptorSets();
    {
        std::unordered_map<VkShaderStageFlagBits, std::string> shaderPaths{
            {VK_SHADER_STAGE_VERTEX_BIT, "generated/shaders/first.vert.spv"},
            {VK_SHADER_STAGE_FRAGMENT_BIT, "generated/shaders/first.frag.spv"},
        };

        GraphicsPipelineBuilder firstPipelineBuilder{};
        m_GraphicsPipeline =
            firstPipelineBuilder
                .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setFaceCulling(VK_CULL_MODE_BACK_BIT,
                                VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .setPolygonMode(VK_POLYGON_MODE_FILL)
                .addColorAttachment(m_Swapchain.format)
                .enableDepthAttachment()
                .build(m_Device, m_BindlessPipelineLayout, shaderPaths);
    }

    initFrameData(m_Swapchain);

    m_StagingBuffer = createStagingBuffer(m_Device, m_Allocator, c_MB * 64);

    m_VertexBuffer =
        createBuffer(m_Device, m_Allocator, c_MB * 64,
                     VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                         VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                         VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                     0);

    std::vector<Vertex> vertices{
        {
            .pos = {-0.5, -0.5, 1.0},
            .u = 0,
            .v = 0,
        },
        {
            .pos = {0.0, -0.5, 1.0},
            .u = 1,
            .v = 0,
        },
        {
            .pos = {-0.5, 0.5, 1.0},
            .u = 0,
            .v = 1,
        },

        {
            .pos = {0.0, -0.5, 1.0},
            .u = 1,
            .v = 0,
        },
        {
            .pos = {0.0, 0.5, 1.0},
            .u = 1,
            .v = 1,
        },
        {
            .pos = {-0.5, 0.5, 1.0},
            .u = 0,
            .v = 1,
        },
    };

    writeToBuffer(m_Device, vertices.data(), vertices.size() * sizeof(Vertex),
                  m_StagingBuffer, m_VertexBuffer);

    stbi_set_flip_vertically_on_load(true);

    int imageWidth{};
    int imageHeight{};
    int channelsInImage{};
    std::string imagePath{"resources/textures/missing_texture.png"};
    stbi_uc *imageBytes{stbi_load(imagePath.c_str(), &imageWidth, &imageHeight,
                                  &channelsInImage, 4)};

    if (imageBytes == nullptr) {
        spdlog::warn("Could not load {}", imagePath);
        return;
    }

    Image image = {createImage(m_Device, m_Allocator, VK_FORMAT_R8G8B8A8_UNORM,
                               {.width = static_cast<uint32_t>(imageWidth),
                                .height = static_cast<uint32_t>(imageHeight)},
                               VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                   VK_IMAGE_USAGE_SAMPLED_BIT,
                               VK_IMAGE_ASPECT_COLOR_BIT)};

    auto cmdBufferRecordCommands{[&](VkCommandBuffer cmdBuffer) {
        transitionImageCmd(
            cmdBuffer, m_Device, image, VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

        writeToImageCmd(m_Device, cmdBuffer, imageBytes, imageWidth,
                        imageHeight, channelsInImage * sizeof(stbi_uc),
                        m_StagingBuffer, image);

        transitionImageCmd(
            cmdBuffer, m_Device, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_SHADER_READ_BIT);
    }};
    submitImmediateCmd(m_TransientCmdPool,
                       m_Device.queues.at(QueueFamily::transfer).handle,
                       m_Device, cmdBufferRecordCommands);

    m_SparseUploadedTextures.emplace_back(image);

    VkSamplerCreateInfo samplerCI{
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = VK_FILTER_NEAREST,
        .minFilter = VK_FILTER_LINEAR,
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy =
            std::min(4.f, m_Device.properties.limits.maxSamplerAnisotropy),
        .maxLod = VK_LOD_CLAMP_NONE};
    vkCreateSampler(m_Device.logical, &samplerCI, nullptr, &m_DefaultSampler);

    VkDescriptorImageInfo descriptorImageInfo{
        .sampler = m_DefaultSampler,
        .imageView = image.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet descriptorSetWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_BindlessDescriptorSet,
        .dstBinding = c_TextureBinding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &descriptorImageInfo,
    };
    vkUpdateDescriptorSets(m_Device.logical, 1, &descriptorSetWrite, 0,
                           nullptr);

    stbi_image_free(imageBytes);
}

void SYN::VK::VulkanBackend::render(Window &window) {
    // will eventually extract currentImageIndex into a seperate
    // "getSurfaceImageAttachment" kindof function to return a handle so that a
    // renderpass can reference that instead of what were doing here
    std::optional<uint32_t> currentImageIndex{beginFrame(window)};
    if (!currentImageIndex.has_value()) {
        spdlog::warn("Could not complete render, beginFrame failed");
        return;
    }

    recordRenderCmd(currentImageIndex.value());

    endFrame(window, currentImageIndex.value());

    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % c_MaxFramesInFlight;
}
void VulkanBackend::shutdown() {
    vkDeviceWaitIdle(m_Device.logical);

    vkDestroySampler(m_Device.logical, m_DefaultSampler, nullptr);
    for (auto &image : m_SparseUploadedTextures) {
        if (image.handle == VK_NULL_HANDLE) {
            continue;
        }
        destroyImage(m_Device, m_Allocator, image);
    }

    for (auto &frame : m_FrameData) {
        vkDestroyFence(m_Device.logical, frame.renderFinishedFence, nullptr);
        vkDestroySemaphore(m_Device.logical, frame.imageAvailableSemaphore,
                           nullptr);
        vkDestroyCommandPool(m_Device.logical, frame.graphicsCmdPool, nullptr);
        destroyImage(m_Device, m_Allocator, frame.depthImage);
    }

    for (const auto &semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device.logical, semaphore, nullptr);
    }

    vkDestroyDescriptorSetLayout(m_Device.logical,
                                 m_BindlessDescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(m_Device.logical, m_BindlessDescriptorPool,
                            nullptr);

    vkDestroyPipelineLayout(m_Device.logical, m_BindlessPipelineLayout,
                            nullptr);
    vkDestroyPipeline(m_Device.logical, m_GraphicsPipeline, nullptr);

    vkDestroyCommandPool(m_Device.logical, m_TransientCmdPool, nullptr);
    destroySwapchain(m_Swapchain, m_Device);
    destroyStagingBuffer(m_StagingBuffer, m_Device, m_Allocator);
    destroyBuffer(m_Allocator, m_VertexBuffer);
    vmaDestroyAllocator(m_Allocator);
    destroyDevice(m_Device);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    destroyDebugMessenger(m_Instance, m_DebugUtilsMessenger);
    vkDestroyInstance(m_Instance, nullptr);
}

void SYN::VK::VulkanBackend::initContext(Window &window) {
    m_Instance = createInstance();

    m_DebugUtilsMessenger = createDebugMessenger(m_Instance);

    VkResult res{glfwCreateWindowSurface(m_Instance, window.getHandle(),
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

    m_Swapchain = createSwapchain(m_Device, m_Surface, window);

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

    VkDescriptorSetLayoutBinding texturesLayoutBinding{
        .binding = c_TextureBinding,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = textureDescriptorCount,
        .stageFlags = VK_SHADER_STAGE_ALL,
    };
    VkDescriptorBindingFlags bindingFlags{
        // VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | // this isnt used yet,
        // will enable when we need it
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCI{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &bindingFlags};

    VkDescriptorSetLayoutCreateInfo bindlessSetLayoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        // .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT
        .pNext = &bindingFlagsCI,
        .bindingCount = 1,
        .pBindings = &texturesLayoutBinding,
    };

    VkResult res{vkCreateDescriptorSetLayout(m_Device.logical,
                                             &bindlessSetLayoutCI, nullptr,
                                             &m_BindlessDescriptorSetLayout)};

    if (res != VK_SUCCESS) {
        spdlog::error("Could not create descriptor set layout. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }
}

void SYN::VK::VulkanBackend::initPipelineLayout() {
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(PushConstants),
    };
    VkPipelineLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &m_BindlessDescriptorSetLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange,
    };

    VkResult res{vkCreatePipelineLayout(m_Device.logical, &layoutCI, nullptr,
                                        &m_BindlessPipelineLayout)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create pipeline layout. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }
}

void SYN::VK::VulkanBackend::initDescriptorSets() {
    uint32_t textureDescriptorCount{
        std::min(c_MaxBindlessTextures,
                 m_Device.properties.limits.maxDescriptorSetSampledImages)};

    VkDescriptorPoolSize combinedSamplersPoolSize{
        .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = textureDescriptorCount,
    };

    VkDescriptorPoolCreateInfo bindlessDescriptorPoolCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        // .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT
        .maxSets = 1,
        .poolSizeCount = 1,
        .pPoolSizes = &combinedSamplersPoolSize};
    VkResult res{vkCreateDescriptorPool(m_Device.logical,
                                        &bindlessDescriptorPoolCI, nullptr,
                                        &m_BindlessDescriptorPool)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create descriptor pool. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }

    VkDescriptorSetAllocateInfo bindlessSetAllocInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_BindlessDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &m_BindlessDescriptorSetLayout,
    };

    res = vkAllocateDescriptorSets(m_Device.logical, &bindlessSetAllocInfo,
                                   &m_BindlessDescriptorSet);

    if (res != VK_SUCCESS) {
        spdlog::error("Could not allocate descriptor sets. VkResult = {}",
                      static_cast<int>(res));
        assert(false);
    }

    m_TextureSlotFreelist.resize(textureDescriptorCount);
    m_SparseUploadedTextures.resize(textureDescriptorCount);
    for (size_t i{}; i < textureDescriptorCount; i++) {
        m_TextureSlotFreelist[i] = i;
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

        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkSemaphore imageAvailableSemaphore{};
        vkCreateSemaphore(m_Device.logical, &semaphoreCI, nullptr,
                          &imageAvailableSemaphore);

        VkFenceCreateInfo fenceCI{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                                  .flags = VK_FENCE_CREATE_SIGNALED_BIT};
        VkFence renderFinishedFence{};
        vkCreateFence(m_Device.logical, &fenceCI, nullptr,
                      &renderFinishedFence);

        Image depthImage{createImage(
            m_Device, m_Allocator, VK_FORMAT_D32_SFLOAT, m_Swapchain.extent,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            VK_IMAGE_ASPECT_DEPTH_BIT)};

        m_FrameData[i] =
            FrameData{.graphicsCmdPool = graphicsCmdPool,
                      .graphicsCmdBuffer = graphicsCmdBuffer,
                      .renderFinishedFence = renderFinishedFence,
                      .imageAvailableSemaphore = imageAvailableSemaphore,
                      .depthImage = depthImage};
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

std::optional<uint32_t> SYN::VK::VulkanBackend::beginFrame(Window &window) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    vkWaitForFences(m_Device.logical, 1, &frame.renderFinishedFence, VK_TRUE,
                    UINT64_MAX);

    uint32_t currentImageIndex{};
    VkResult res{vkAcquireNextImageKHR(
        m_Device.logical, m_Swapchain.handle, UINT64_MAX,
        frame.imageAvailableSemaphore, VK_NULL_HANDLE, &currentImageIndex)};

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        spdlog::info("Recreating swapchain");
        recreateSwapchain(window);
        return {};
    }

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        spdlog::error("Could not acquire swap chain image. VkResult = {}",
                      static_cast<int>(res));
    }
    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkResetCommandPool(m_Device.logical, frame.graphicsCmdPool, 0);
    vkBeginCommandBuffer(frame.graphicsCmdBuffer, &cmdBufferBeginInfo);

    return currentImageIndex;
}

void SYN::VK::VulkanBackend::recordRenderCmd(uint32_t currentImageIndex) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    VkImageMemoryBarrier2 colorAttachmentBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex =
            m_Device.queues[QueueFamily::graphics].familyIndex,
        .dstQueueFamilyIndex =
            m_Device.queues[QueueFamily::graphics].familyIndex,
        .image = m_Swapchain.images[currentImageIndex],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        }};

    VkImageMemoryBarrier2 depthAttachmentBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_2_NONE,
        .dstStageMask = VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                        VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
        .dstAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                         VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
        .srcQueueFamilyIndex =
            m_Device.queues[QueueFamily::graphics].familyIndex,
        .dstQueueFamilyIndex =
            m_Device.queues[QueueFamily::graphics].familyIndex,
        .image = frame.depthImage.handle,
        .subresourceRange = frame.depthImage.subresourceRange};

    std::vector<VkImageMemoryBarrier2> imageMemoryBarriers{
        colorAttachmentBarrier, depthAttachmentBarrier};

    VkDependencyInfo colorAttachmentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount =
            static_cast<uint32_t>(imageMemoryBarriers.size()),
        .pImageMemoryBarriers = imageMemoryBarriers.data(),
    };
    vkCmdPipelineBarrier2(frame.graphicsCmdBuffer, &colorAttachmentDependency);

    Image swapchainImage{.handle = m_Swapchain.images[currentImageIndex],
                         .view = m_Swapchain.imageViews[currentImageIndex],
                         .extent = m_Swapchain.extent,
                         .subresourceRange =
                             colorAttachmentBarrier.subresourceRange};

    PushConstants pushConstants{.vertexBuffer = m_VertexBuffer.deviceAddress};

    uint32_t nVertices{6};
    defaultRenderPassCmd(
        frame.graphicsCmdBuffer, m_GraphicsPipeline, m_BindlessPipelineLayout,
        pushConstants, m_BindlessDescriptorSet, nVertices, swapchainImage,
        frame.depthImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    VkImageMemoryBarrier2 presentBarrier{
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
        .dstAccessMask = VK_ACCESS_2_NONE,
        .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex =
            m_Device.queues[QueueFamily::graphics].familyIndex,
        .dstQueueFamilyIndex =
            m_Device.queues[QueueFamily::present].familyIndex,
        .image = m_Swapchain.images[currentImageIndex],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        }};

    VkDependencyInfo presentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &presentBarrier,
    };
    vkCmdPipelineBarrier2(frame.graphicsCmdBuffer, &presentDependency);
}
void SYN::VK::VulkanBackend::endFrame(Window &window,
                                      uint32_t currentImageIndex) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    vkEndCommandBuffer(frame.graphicsCmdBuffer);

    VkPipelineStageFlags waitStage{
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo queueSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &frame.imageAvailableSemaphore,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &frame.graphicsCmdBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_RenderFinishedSemaphores[currentImageIndex],
    };

    vkResetFences(m_Device.logical, 1, &frame.renderFinishedFence);
    vkQueueSubmit(m_Device.queues[QueueFamily::graphics].handle, 1,
                  &queueSubmitInfo, frame.renderFinishedFence);

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_RenderFinishedSemaphores[currentImageIndex],
        .swapchainCount = 1,
        .pSwapchains = &m_Swapchain.handle,
        .pImageIndices = &currentImageIndex,
    };

    VkResult res{vkQueuePresentKHR(m_Device.queues[QueueFamily::present].handle,
                                   &presentInfo)};

    if ((res == VK_SUBOPTIMAL_KHR) || (res == VK_ERROR_OUT_OF_DATE_KHR)) {
        spdlog::info("Recreating swapchain");
        recreateSwapchain(window);
    } else if (res != VK_SUCCESS) {
        spdlog::error("Could not present image. VkResult = {}",
                      static_cast<int>(res));
    }
}

// note this fella dont work yet
void SYN::VK::VulkanBackend::updateTextures(
    std::span<size_t> textureAssetIDs,
    const std::unordered_map<size_t, LoadedImage> &assetIDToLoadedImage) {

    assert(std::is_sorted(m_TextureSlotFreelist.begin(),
                          m_TextureSlotFreelist.end()));

    std::unordered_set<size_t> requiredAssetIDs(textureAssetIDs.begin(),
                                                textureAssetIDs.end());

    {
        std::vector<size_t> staleAssetIDs{};

        auto uploadedAssetIDs{std::views::keys(m_AssetIDToTextureSlot)};

        for (const auto &[id, slot] : m_AssetIDToTextureSlot) {
            if (!requiredAssetIDs.contains(id))
                staleAssetIDs.push_back(id);
        }

        std::vector<size_t> newFreeTextureSlots{};
        for (auto id : staleAssetIDs) {
            size_t textureSlot{m_AssetIDToTextureSlot.at(id)};
            newFreeTextureSlots.emplace_back(textureSlot);
            m_AssetIDToTextureSlot.erase(id);
            destroyImage(m_Device, m_Allocator,
                         m_SparseUploadedTextures[textureSlot]);
        }
        std::sort(newFreeTextureSlots.begin(), newFreeTextureSlots.end());

        std::vector<size_t> newFreelist{};
        newFreelist.reserve(newFreeTextureSlots.size() +
                            m_TextureSlotFreelist.size());
        std::merge(m_TextureSlotFreelist.begin(), m_TextureSlotFreelist.end(),
                   newFreeTextureSlots.begin(), newFreeTextureSlots.end(),
                   std::back_inserter(newFreelist));

        m_TextureSlotFreelist = std::move(newFreelist);
    }

    auto uploadedAssetIDs{std::views::keys(m_AssetIDToTextureSlot)};

    std::vector<size_t> assetIDsToLoad{};
    for (const auto &id : requiredAssetIDs) {
        if (!m_AssetIDToTextureSlot.contains(id))
            assetIDsToLoad.push_back(id);
    }

    std::vector<VkWriteDescriptorSet> descriptorWrites{};
    descriptorWrites.reserve(assetIDsToLoad.size());

    std::vector<VkDescriptorImageInfo> descriptorImageInfos{};
    descriptorImageInfos.reserve(assetIDsToLoad.size());

    for (size_t i{}; i < assetIDsToLoad.size(); i++) {
        if (m_TextureSlotFreelist.empty()) {
            spdlog::warn("Could not upload texture, all slots are being used");
            break;
        }
        size_t textureSlot{m_TextureSlotFreelist.back()};
        m_TextureSlotFreelist.pop_back();

        size_t id{assetIDsToLoad[i]};
        m_AssetIDToTextureSlot[id] = textureSlot;

        const LoadedImage &loadedImage{assetIDToLoadedImage.at(id)};

        {
            Image image = {createImage(
                m_Device, m_Allocator, VK_FORMAT_R8G8B8A8_UNORM,
                {.width = static_cast<uint32_t>(loadedImage.width),
                 .height = static_cast<uint32_t>(loadedImage.height)},
                VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                VK_IMAGE_ASPECT_COLOR_BIT)};

            VkCommandBuffer cmdBuffer{
                beginTransientCmd(m_TransientCmdPool, m_Device)};

            transitionImageCmd(
                cmdBuffer, m_Device, image, VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT);

            writeToImageCmd(m_Device, cmdBuffer, loadedImage.data,
                            loadedImage.width, loadedImage.height,
                            loadedImage.channels * sizeof(uint8_t),
                            m_StagingBuffer, image);

            transitionImageCmd(cmdBuffer, m_Device, image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                               VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                               VK_ACCESS_2_TRANSFER_WRITE_BIT,
                               VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT,
                               VK_ACCESS_2_SHADER_READ_BIT);

            endTransientCmd(m_TransientCmdPool, cmdBuffer, m_Device,
                            m_Device.queues.at(QueueFamily::transfer).handle);

            m_SparseUploadedTextures[textureSlot] = std::move(image);
        }

        descriptorImageInfos.emplace_back(VkDescriptorImageInfo{
            .sampler = m_DefaultSampler,
            .imageView = m_SparseUploadedTextures[textureSlot].view,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        });

        descriptorWrites.emplace_back(VkWriteDescriptorSet{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_BindlessDescriptorSet,
            .dstBinding = c_TextureBinding,
            .dstArrayElement = static_cast<uint32_t>(textureSlot),
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &descriptorImageInfos.back(),
        });
    }

    vkUpdateDescriptorSets(m_Device.logical, descriptorWrites.size(),
                           descriptorWrites.data(), 0, nullptr);
}

void SYN::VK::VulkanBackend::recreateSwapchain(Window &window) {
    vkDeviceWaitIdle(m_Device.logical);

    Swapchain newSwapchain{
        createSwapchain(m_Device, m_Surface, window, m_Swapchain.handle)};
    destroySwapchain(m_Swapchain, m_Device);
    m_Swapchain = newSwapchain;
}
