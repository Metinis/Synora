#include "Core.h"
#include "Buffer.h"
#include "Commands.h"
#include "DebugMessenger.h"
#include "Instance.h"
#include "Pipeline.h"
#include "StagingBuffer.h"
#include "Swapchain.h"
#include "renderer/backends/vulkan/Device.h"
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

#include "PuzzleEngine/project/Assets.h"
#include "PuzzleEngine/scene/Components.h"

using namespace SYN;
using namespace SYN::VK;

namespace {
constexpr uint32_t c_MB{1024 * 1024};

VkAttachmentLoadOp toVkLoadOp(LoadOp loadOp);
VkAttachmentStoreOp toVkStoreOp(StoreOp loadOp);

VkRenderingAttachmentInfo makeAttachmentInfo(const Image &image,
                                             const WriteAttachment &attachment,
                                             VkImageLayout targetLayout);

struct StageAccess {
    VkPipelineStageFlags2 stageMask;
    VkAccessFlags2 accessMask;
};

StageAccess getStageAccess(VkImageLayout layout);

VkImageMemoryBarrier2 makeImageMemoryBarrier(const Image &image,
                                             VkImageLayout targetLayout,
                                             uint32_t srcQueueFamilyIndex = 0,
                                             uint32_t dstQueueFamilyIndex = 0);

struct PushConstants {
    VkDeviceAddress vertexBuffer;
};

} // namespace

void SYN::VK::VulkanBackend::init(Window *window) {
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
                .build(m_Device, m_PipelineLayout, shaderPaths);
    }

    initFrameData(m_Swapchain);

    m_StagingBuffer = StagingBuffer().create(m_Device, m_Allocator, c_MB * 64);

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
}

BufferHandle SYN::VK::VulkanBackend::createBuffer(const BufferDesc &desc) {
    Buffer buffer{
        VK::createBuffer(m_Device, m_Allocator, desc.size,
                         VK_BUFFER_USAGE_2_TRANSFER_DST_BIT |
                             VK_BUFFER_USAGE_2_SHADER_DEVICE_ADDRESS_BIT |
                             VK_BUFFER_USAGE_2_STORAGE_BUFFER_BIT,
                         0)};

    BufferHandle handle{m_Buffers.insert(buffer)};

    return handle;
}

void SYN::VK::VulkanBackend::uploadToBuffer(BufferHandle handle, size_t size,
                                            void *data) {
    const Buffer &buffer{m_Buffers[handle]};
    m_StagingBuffer.uploadToBuffer(m_Device, data, size, buffer);
}

void SYN::VK::VulkanBackend::destroyBuffer(BufferHandle &handle) {
    // very bad, use a deletion queue or something else later on
    vkDeviceWaitIdle(m_Device.logical);
    Buffer buffer{m_Buffers[handle]};

    m_Buffers.remove(handle);

    handle.id = UINT32_MAX;
}

TextureHandle SYN::VK::VulkanBackend::createTexture(const TextureDesc &desc) {
    VkFormat format{VK_FORMAT_R8G8B8A8_UNORM};
    VkImageUsageFlags usage{VK_IMAGE_USAGE_TRANSFER_DST_BIT};
    VkImageAspectFlags aspect{};
    switch (desc.type) {
    case TextureType::srgb:
        format = VK_FORMAT_R8G8B8A8_SRGB;
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    case TextureType::depth:
        format = VK_FORMAT_D32_SFLOAT;
        usage |=
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT; // will need to change
                                                         // this, good nuff for
                                                         // now
        aspect = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
    case TextureType::rgba:
        format = VK_FORMAT_R8G8B8A8_UNORM;
        usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        aspect = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    default:
        spdlog::warn("Could not add texture, invalid format");
        break;
    }

    Image texture{createImage(m_Device, m_Allocator, format,
                              {.width = desc.width, .height = desc.height},
                              usage, aspect)};

    TextureHandle handle{m_Textures.insert(texture)};

    return handle;
}

void SYN::VK::VulkanBackend::uploadToTexture(TextureHandle handle,
                                             uint32_t width, uint32_t height,
                                             uint32_t stride, void *data) {
    Image &texture{m_Textures[handle]};

    // TODO: have the transition happen automatically for all image types
    transitionImage(
        m_TransientCmdPool, m_Device, texture, VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT);

    m_StagingBuffer.uploadToImage(m_Device, data, width, height, stride,
                                  texture);

    transitionImage(
        m_TransientCmdPool, m_Device, texture,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
        VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, VK_ACCESS_2_SHADER_READ_BIT);

    VkDescriptorImageInfo imageInfo{
        .sampler = m_DefaultSampler,
        .imageView = texture.view,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet bindlessWrite{
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = m_BindlessDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo,
    };

    vkUpdateDescriptorSets(m_Device.logical, 1, &bindlessWrite, 0, nullptr);
}

void SYN::VK::VulkanBackend::destroyTexture(TextureHandle &handle) {
    // VERY BAD, temporary, will make this better later
    vkDeviceWaitIdle(m_Device.logical);
    Image texture{m_Textures[handle]};

    destroyImage(m_Device, m_Allocator, texture);
    handle.id = UINT32_MAX;
}

void SYN::VK::VulkanBackend::shutdown() {
    vkDeviceWaitIdle(m_Device.logical);

    vkDestroySampler(m_Device.logical, m_DefaultSampler, nullptr);

    for (auto [handle, texture] : m_Textures) {
        spdlog::warn("Texture {} may have been leaked", handle.id);
        destroyImage(m_Device, m_Allocator, texture);
    }

    for (auto [handle, buffer] : m_Buffers) {
        spdlog::warn("Buffer {} may have been leaked", handle.id);
        VK::destroyBuffer(m_Allocator, buffer);
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

    vkDestroyDescriptorSetLayout(m_Device.logical,
                                 m_BindlessDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(m_Device.logical, m_UBODescriptorSetLayout,
                                 nullptr);
    vkDestroyDescriptorPool(m_Device.logical, m_DescriptorPool, nullptr);

    vkDestroyPipelineLayout(m_Device.logical, m_PipelineLayout, nullptr);
    vkDestroyPipeline(m_Device.logical, m_GraphicsPipeline, nullptr);

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

    VkDescriptorBindingFlags bindingFlags{
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT};

    VkDescriptorSetLayoutBindingFlagsCreateInfo bindingFlagsCI{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &bindingFlags};

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
        .pNext = &bindingFlagsCI,
        .bindingCount = 1,
        .pBindings = &bindlessTextureBinding,
    };
    VkDescriptorSetLayoutCreateInfo uboLayoutCI{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
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

void SYN::VK::VulkanBackend::initPipelineLayout() {
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_ALL,
        .size = sizeof(PushConstants),
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
                                        &m_PipelineLayout)};
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

void SYN::VK::VulkanBackend::beginFrame(Window &window) {
    m_StagingBuffer.stallOnPendingUploads(m_Device);
    FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    vkWaitForFences(m_Device.logical, 1, &frame.renderFinishedFence, VK_TRUE,
                    UINT64_MAX);

    VkResult res{
        vkAcquireNextImageKHR(m_Device.logical, m_Swapchain.handle, UINT64_MAX,
                              frame.imageAvailableSemaphore, VK_NULL_HANDLE,
                              &frame.swapchainImageIndex)};

    if (res == VK_ERROR_OUT_OF_DATE_KHR) {
        spdlog::info("Recreating swapchain");
        recreateSwapchain(window);
        return;
    }

    if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR) {
        spdlog::error("Could not acquire swap chain image. VkResult = {}",
                      static_cast<int>(res));
    }

    Image swapchainImage{
        .handle = m_Swapchain.images[frame.swapchainImageIndex],
        .view = m_Swapchain.imageViews[frame.swapchainImageIndex],
        .extent = m_Swapchain.extent,
        .subresourceRange =
            VkImageSubresourceRange{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        .currentLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    frame.swapchainImageHandle = m_Textures.insert(std::move(swapchainImage));

    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT};
    vkResetCommandPool(m_Device.logical, frame.graphicsCmdPool, 0);
    vkBeginCommandBuffer(frame.graphicsCmdBuffer, &cmdBufferBeginInfo);
    vkCmdBindDescriptorSets(frame.graphicsCmdBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout,
                            0, 1, &m_BindlessDescriptorSet, 0, nullptr);
}

void SYN::VK::VulkanBackend::endFrame(Window &window) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    VkImageMemoryBarrier2 presentBarrier{
        makeImageMemoryBarrier(m_Textures[frame.swapchainImageHandle],
                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)};

    VkDependencyInfo presentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &presentBarrier,
    };
    vkCmdPipelineBarrier2(frame.graphicsCmdBuffer, &presentDependency);

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
        .pSignalSemaphores =
            &m_RenderFinishedSemaphores[frame.swapchainImageIndex],
    };

    vkResetFences(m_Device.logical, 1, &frame.renderFinishedFence);
    vkQueueSubmit(m_Device.queues[QueueFamily::graphics].handle, 1,
                  &queueSubmitInfo, frame.renderFinishedFence);

    VkPresentInfoKHR presentInfo{
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores =
            &m_RenderFinishedSemaphores[frame.swapchainImageIndex],
        .swapchainCount = 1,
        .pSwapchains = &m_Swapchain.handle,
        .pImageIndices = &frame.swapchainImageIndex,
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

    m_Textures.remove(frame.swapchainImageHandle);
    m_CurrentFrameIndex = (m_CurrentFrameIndex + 1) % c_MaxFramesInFlight;
}

RenderPassHandle
SYN::VK::VulkanBackend::beginRenderPassCmd(const RenderPassDesc &desc) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    std::vector<VkImageMemoryBarrier2> attachmentBarriers{};

    std::vector<VkRenderingAttachmentInfo> colorAttachmentInfos;
    colorAttachmentInfos.reserve(desc.colorAttachments.size());
    VkRenderingAttachmentInfo depthAttachmentInfo{};

    for (const auto &attachment : desc.colorAttachments) {
        Image &image{m_Textures[attachment.textureHandle]};
        VkImageLayout targetLayout{VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};
        colorAttachmentInfos.emplace_back(
            makeAttachmentInfo(image, attachment, targetLayout));

        if (image.currentLayout != targetLayout) {
            attachmentBarriers.emplace_back(
                makeImageMemoryBarrier(image, targetLayout));
            image.currentLayout = targetLayout;
        }
    }
    if (desc.depthAttachment.has_value()) {
        WriteAttachment attachment{desc.depthAttachment.value()};
        Image &image{m_Textures[attachment.textureHandle]};
        VkImageLayout targetLayout{VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL};
        depthAttachmentInfo =
            makeAttachmentInfo(image, attachment, targetLayout);

        if (image.currentLayout != targetLayout) {

            attachmentBarriers.emplace_back(
                makeImageMemoryBarrier(image, targetLayout));
            image.currentLayout = targetLayout;
        }
    }

    for (const auto &handle : desc.readAttachments) {
        Image &image{m_Textures[handle]};
        VkImageLayout targetLayout{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

        if (image.currentLayout != targetLayout) {
            attachmentBarriers.emplace_back(
                makeImageMemoryBarrier(image, targetLayout));
            image.currentLayout = targetLayout;
        }
    }

    VkDependencyInfo colorAttachmentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount =
            static_cast<uint32_t>(attachmentBarriers.size()),
        .pImageMemoryBarriers = attachmentBarriers.data(),
    };

    vkCmdPipelineBarrier2(frame.graphicsCmdBuffer, &colorAttachmentDependency);

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = VkRect2D{.extent =
                                   VkExtent2D{
                                       .width = desc.viewport.width,
                                       .height = desc.viewport.height,
                                   }},
        .layerCount = 1,
        .colorAttachmentCount =
            static_cast<uint32_t>(colorAttachmentInfos.size()),
        .pColorAttachments = colorAttachmentInfos.data(),
    };
    if (desc.depthAttachment.has_value()) {
        renderingInfo.pDepthAttachment = &depthAttachmentInfo;
    }

    vkCmdBeginRendering(frame.graphicsCmdBuffer, &renderingInfo);

    VkViewport viewport{.y = static_cast<float>(desc.viewport.height),
                        .width = static_cast<float>(desc.viewport.width),
                        .height = -static_cast<float>(desc.viewport.height),
                        .minDepth = 0.f,
                        .maxDepth = 1.f};

    VkRect2D scissor{.extent = VkExtent2D{
                         .width = desc.viewport.width,
                         .height = desc.viewport.height,
                     }};

    vkCmdSetViewport(frame.graphicsCmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(frame.graphicsCmdBuffer, 0, 1, &scissor);
    // TODO: add a way to create pipelines and use those instead
    vkCmdBindPipeline(frame.graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_GraphicsPipeline);
    // TODO: later on maybe we wanna add some per render pass stuff?
    return {};
}
void SYN::VK::VulkanBackend::endRenderPassCmd(RenderPassHandle &renderPass) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    vkCmdEndRendering(frame.graphicsCmdBuffer);
}

void SYN::VK::VulkanBackend::drawCmd(BufferHandle vertexBufferHandle,
                                     size_t nVertices) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    Buffer vertexBuffer{m_Buffers[vertexBufferHandle]};
    PushConstants pushConstants{.vertexBuffer = vertexBuffer.deviceAddress};

    vkCmdPushConstants(frame.graphicsCmdBuffer, m_PipelineLayout,
                       VK_SHADER_STAGE_ALL, 0, sizeof(PushConstants),
                       &pushConstants);

    vkCmdDraw(frame.graphicsCmdBuffer, nVertices, 1, 0, 0);
}

TextureHandle SYN::VK::VulkanBackend::getSwapchainTextureCmd() {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};
    return frame.swapchainImageHandle;
}

Viewport SYN::VK::VulkanBackend::getSwapchainViewport() {
    return Viewport{.width = m_Swapchain.extent.width,
                    .height = m_Swapchain.extent.height};
}

void SYN::VK::VulkanBackend::recreateSwapchain(Window &window) {
    vkDeviceWaitIdle(m_Device.logical);

    Swapchain newSwapchain{
        createSwapchain(m_Device, m_Surface, window, m_Swapchain.handle)};
    destroySwapchain(m_Swapchain, m_Device);
    m_Swapchain = newSwapchain;
}

namespace {
VkAttachmentLoadOp toVkLoadOp(LoadOp loadOp) {
    VkAttachmentLoadOp vkLoadOp{};
    switch (loadOp) {
    case LoadOp::clear:
        vkLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        break;
    case LoadOp::load:
        vkLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        break;
    case LoadOp::dontCare:
        vkLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        break;
    default:
        break;
    }
    return vkLoadOp;
}
VkAttachmentStoreOp toVkStoreOp(StoreOp storeOp) {
    VkAttachmentStoreOp vkStoreOp{};
    switch (storeOp) {
    case StoreOp::store:
        vkStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        break;
    case StoreOp::dontCare:
        vkStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        break;
    default:
        break;
    }
    return vkStoreOp;
}

VkRenderingAttachmentInfo makeAttachmentInfo(const Image &image,
                                             const WriteAttachment &attachment,
                                             VkImageLayout targetLayout) {
    VkClearValue clearValue{};
    if (image.subresourceRange.aspectMask == VK_IMAGE_ASPECT_COLOR_BIT) {
        clearValue = {.color = VkClearColorValue{
                          attachment.clearColor.r,
                          attachment.clearColor.g,
                          attachment.clearColor.b,
                          attachment.clearColor.a,
                      }};
    } else {
        clearValue = {.depthStencil = VkClearDepthStencilValue{
                          .depth = attachment.clearDepth}};
    }

    VkAttachmentLoadOp loadOp{toVkLoadOp(attachment.loadOp)};
    VkAttachmentStoreOp storeOp{toVkStoreOp(attachment.storeOp)};

    VkRenderingAttachmentInfo attachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = image.view,
        .imageLayout = targetLayout,
        .loadOp = loadOp,
        .storeOp = storeOp,
        .clearValue = clearValue,
    };

    return attachmentInfo;
}

StageAccess getStageAccess(VkImageLayout layout) {
    switch (layout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        return {VK_PIPELINE_STAGE_2_NONE, VK_ACCESS_2_NONE};
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
                    VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT,
                VK_ACCESS_2_SHADER_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_READ_BIT};
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return {VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT};
    case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
        return {VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, VK_ACCESS_2_NONE};
    default:
        spdlog::warn("Unspecified layout encountered");
        assert(false);
        return {VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
                VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT};
    }
}

VkImageMemoryBarrier2 makeImageMemoryBarrier(const Image &image,
                                             VkImageLayout targetLayout,
                                             uint32_t srcQueueFamilyIndex,
                                             uint32_t dstQueueFamilyIndex) {
    StageAccess srcStageAccess{getStageAccess(image.currentLayout)};
    StageAccess dstStageAccess{getStageAccess(targetLayout)};
    return VkImageMemoryBarrier2{.sType =
                                     VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                 .srcStageMask = srcStageAccess.stageMask,
                                 .srcAccessMask = srcStageAccess.accessMask,
                                 .dstStageMask = dstStageAccess.stageMask,
                                 .dstAccessMask = dstStageAccess.accessMask,
                                 .oldLayout = image.currentLayout,
                                 .newLayout = targetLayout,
                                 .srcQueueFamilyIndex = srcQueueFamilyIndex,
                                 .dstQueueFamilyIndex = dstQueueFamilyIndex,
                                 .image = image.handle,
                                 .subresourceRange = image.subresourceRange};
}
} // namespace
