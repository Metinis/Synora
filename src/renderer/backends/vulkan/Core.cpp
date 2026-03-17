#include "Core.h"
#include "Buffer.h"
#include "Commands.h"
#include "DebugMessenger.h"
#include "Image.h"
#include "Instance.h"
#include "Pipeline.h"
#include "Renderpass.h"
#include "StagingBuffer.h"
#include "Swapchain.h"

#include <GLFW/glfw3.h>
#include <cstring>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <vk_mem_alloc.h>

#include <PuzzleEngine/core/Window.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

namespace {
constexpr uint32_t c_MB{1024 * 1024};

VkShaderModule createShaderModule(const Device &device,
                                  const std::string &path);
} // namespace

void VulkanBackend::init(SYN::Window &window) {
    initContext(window);

    initBindlessPipelineLayout();
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
        {.pos = {-0.5, -0.5, 1.0}}, {.pos = {0.0, -0.5, 1.0}},
        {.pos = {-0.5, 0.5, 1.0}},

        {.pos = {0.0, -0.5, 1.0}},  {.pos = {0.0, 0.5, 1.0}},
        {.pos = {-0.5, 0.5, 1.0}},
    };

    writeToBuffer(m_Device, vertices.data(), vertices.size() * sizeof(Vertex),
                  m_StagingBuffer, m_VertexBuffer);
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

    for (const auto &frame : m_FrameData) {
        vkDestroyFence(m_Device.logical, frame.renderFinishedFence, nullptr);
        vkDestroySemaphore(m_Device.logical, frame.imageAvailableSemaphore,
                           nullptr);
        vkDestroyCommandPool(m_Device.logical, frame.graphicsCmdPool, nullptr);
    }

    for (const auto &semaphore : m_RenderFinishedSemaphores) {
        vkDestroySemaphore(m_Device.logical, semaphore, nullptr);
    }

    vkDestroyPipelineLayout(m_Device.logical, m_BindlessPipelineLayout,
                            nullptr);
    vkDestroyPipeline(m_Device.logical, m_GraphicsPipeline, nullptr);

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
}

void SYN::VK::VulkanBackend::initBindlessPipelineLayout() {
    VkPushConstantRange pushConstantRange{
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
        .size = sizeof(PushConstants),
    };

    VkPipelineLayoutCreateInfo layoutCI{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange};

    vkCreatePipelineLayout(m_Device.logical, &layoutCI, nullptr,
                           &m_BindlessPipelineLayout);
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

        m_FrameData[i] =
            FrameData{.graphicsCmdPool = graphicsCmdPool,
                      .graphicsCmdBuffer = graphicsCmdBuffer,
                      .renderFinishedFence = renderFinishedFence,
                      .imageAvailableSemaphore = imageAvailableSemaphore};
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

    VkDependencyInfo colorAttachmentDependency{
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &colorAttachmentBarrier,
    };
    vkCmdPipelineBarrier2(frame.graphicsCmdBuffer, &colorAttachmentDependency);

    Image swapchainImage{.handle = m_Swapchain.images[currentImageIndex],
                         .view = m_Swapchain.imageViews[currentImageIndex],
                         .extent = m_Swapchain.extent,
                         .subresourceRange =
                             colorAttachmentBarrier.subresourceRange};

    PushConstants pushConstants{.vertexBuffer = m_VertexBuffer.deviceAddress};

    uint32_t nVertices{6};
    cmdDefaultRenderPass(frame.graphicsCmdBuffer, m_GraphicsPipeline,
                         m_BindlessPipelineLayout, pushConstants, nVertices,
                         swapchainImage,
                         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

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

void SYN::VK::VulkanBackend::recreateSwapchain(Window &window) {
    vkDeviceWaitIdle(m_Device.logical);

    Swapchain newSwapchain{
        createSwapchain(m_Device, m_Surface, window, m_Swapchain.handle)};
    destroySwapchain(m_Swapchain, m_Device);
    m_Swapchain = newSwapchain;
}
