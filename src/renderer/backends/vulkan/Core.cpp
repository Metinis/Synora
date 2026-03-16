#include "Core.h"
#include "DebugMessenger.h"
#include "Instance.h"
#include "Pipeline.h"
#include "Swapchain.h"

#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <spdlog/spdlog.h>

#include <PuzzleEngine/core/Window.h>
#include <vulkan/vulkan_core.h>

using namespace SYN;
using namespace SYN::VK;

namespace {
VkShaderModule createShaderModule(const Device &device,
                                  const std::string &path);
}

void VulkanBackend::init(SYN::Window &window) {
    m_Instance = createInstance();

    m_DebugUtilsMessenger = createDebugMessenger(m_Instance);

    VkResult res{glfwCreateWindowSurface(m_Instance, window.getHandle(),
                                         nullptr, &m_Surface)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create Vulkan surface");
    }

    m_Device = createDevice(m_Instance, m_Surface);
    m_Swapchain = createSwapchain(m_Device, m_Surface, window);

    {
        VkShaderModule vertShaderModule{
            createShaderModule(m_Device, "generated/shaders/first.vert.spv")};

        VkShaderModule fragShaderModule{
            createShaderModule(m_Device, "generated/shaders/first.frag.spv")};

        VkPipelineLayoutCreateInfo layoutCI{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        };
        vkCreatePipelineLayout(m_Device.logical, &layoutCI, nullptr,
                               &m_GraphicsPipelineLayout);
        GraphicsPipelineBuilder graphicsPipelineBuilder{};
        m_GraphicsPipeline =
            graphicsPipelineBuilder
                .setShaderStage(fragShaderModule, VK_SHADER_STAGE_FRAGMENT_BIT)
                .setShaderStage(vertShaderModule, VK_SHADER_STAGE_VERTEX_BIT)
                .setInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
                .setFaceCulling(VK_CULL_MODE_BACK_BIT,
                                VK_FRONT_FACE_COUNTER_CLOCKWISE)
                .setPolygonMode(VK_POLYGON_MODE_FILL)
                .addColorAttachment(m_Swapchain.format)
                .disableDepthTest()
                .disableMultisampling()
                .build(m_Device, m_GraphicsPipelineLayout);

        vkDestroyShaderModule(m_Device.logical, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_Device.logical, fragShaderModule, nullptr);
    }

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
        res = vkAllocateCommandBuffers(m_Device.logical, &cmdBufferAllocInfo,
                                       &graphicsCmdBuffer);
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
    for (const auto &_ : m_Swapchain.images) {
        VkSemaphoreCreateInfo semaphoreCI{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkSemaphore renderFinishedSemaphore{};
        vkCreateSemaphore(m_Device.logical, &semaphoreCI, nullptr,
                          &renderFinishedSemaphore);
        m_RenderFinishedSemaphores.emplace_back(renderFinishedSemaphore);
    }
}

void SYN::VK::VulkanBackend::render(Window &window) {
    const FrameData &frame{m_FrameData[m_CurrentFrameIndex]};

    vkWaitForFences(m_Device.logical, 1, &frame.renderFinishedFence, VK_TRUE,
                    UINT64_MAX);

    uint32_t currentImageIndex{};
    VkResult res{vkAcquireNextImageKHR(
        m_Device.logical, m_Swapchain.handle, UINT64_MAX,
        frame.imageAvailableSemaphore, VK_NULL_HANDLE, &currentImageIndex)};

    VkCommandBufferBeginInfo cmdBufferBeginInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    };
    vkResetCommandPool(m_Device.logical, frame.graphicsCmdPool, 0);

    vkBeginCommandBuffer(frame.graphicsCmdBuffer, &cmdBufferBeginInfo);

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

    constexpr VkClearValue colorClearValue{
        .color = VkClearColorValue{{0.f, 0.f, 0.f, 0.f}}};

    VkRenderingAttachmentInfo colorAttachmentInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = m_Swapchain.imageViews[currentImageIndex],
        .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = colorClearValue,
    };

    VkRenderingInfo renderingInfo{
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = VkRect2D{.extent = m_Swapchain.extent},
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentInfo,

    };
    vkCmdBeginRendering(frame.graphicsCmdBuffer, &renderingInfo);
    vkCmdBindPipeline(frame.graphicsCmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      m_GraphicsPipeline);

    VkViewport viewport{.y = static_cast<float>(m_Swapchain.extent.height),
                        .width = static_cast<float>(m_Swapchain.extent.width),
                        .height =
                            -static_cast<float>(m_Swapchain.extent.height),
                        .minDepth = 0.f,
                        .maxDepth = 1.f};

    VkRect2D scissor{.extent = m_Swapchain.extent};

    vkCmdSetViewport(frame.graphicsCmdBuffer, 0, 1, &viewport);
    vkCmdSetScissor(frame.graphicsCmdBuffer, 0, 1, &scissor);

    vkCmdDraw(frame.graphicsCmdBuffer, 3, 1, 0, 0);
    vkCmdEndRendering(frame.graphicsCmdBuffer);

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

    vkEndCommandBuffer(frame.graphicsCmdBuffer);

    VkCommandBufferSubmitInfo cmdBufferSubmitInfo{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = frame.graphicsCmdBuffer,
    };

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

    res = vkQueuePresentKHR(m_Device.queues[QueueFamily::present].handle,
                            &presentInfo);

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

    vkDestroyPipelineLayout(m_Device.logical, m_GraphicsPipelineLayout,
                            nullptr);
    vkDestroyPipeline(m_Device.logical, m_GraphicsPipeline, nullptr);

    destroySwapchain(&m_Swapchain, m_Device);
    destroyDevice(&m_Device);
    vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
    destroyDebugMessenger(m_Instance, m_DebugUtilsMessenger);
    vkDestroyInstance(m_Instance, nullptr);
}

// TODO: add default shaders for if theres an error
namespace {
VkShaderModule createShaderModule(const Device &device,
                                  const std::string &path) {
    std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        spdlog::error("Could not open {}, shader module creation failed", path);
        assert(false);
        return VK_NULL_HANDLE;
    }
    std::streampos fileSize{file.tellg()};
    if (fileSize < 0) {
        spdlog::error("Tellg failed, shader module creation failed", path);
        assert(false);
        return VK_NULL_HANDLE;
    }

    if ((fileSize % sizeof(uint32_t)) != 0) {
        spdlog::error("Could not create shader module, {} file size was not a "
                      "multiple of 32 "
                      "and may be malformed",
                      path);
        assert(false);
        return VK_NULL_HANDLE;
    }

    std::vector<uint32_t> shaderCode(fileSize / sizeof(uint32_t));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char *>(shaderCode.data()), fileSize);

    VkShaderModuleCreateInfo shaderModuleCI{
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderCode.size() * sizeof(uint32_t), // byte size
        .pCode = shaderCode.data()};

    VkShaderModule shaderModule{};
    VkResult res{vkCreateShaderModule(device.logical, &shaderModuleCI, nullptr,
                                      &shaderModule)};
    if (res != VK_SUCCESS) {
        spdlog::error("Could not create shader module of {},  VkResult = {}",
                      path, static_cast<int>(res));
        assert(false);
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}
} // namespace
