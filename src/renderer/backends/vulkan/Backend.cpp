#include "Backend.h"
#include "DebugMessenger.h"
#include "Instance.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <PuzzleEngine/core/Window.h>

using namespace SYN;

void SYN::VulkanBackend::init(Window &window) {
    VkInstance instance{createInstance()};

    VkSurfaceKHR surface{};
    VkResult res{glfwCreateWindowSurface(instance, window.getHandle(), nullptr,
                                         &surface)};
    if (res != VK_SUCCESS) {
        spdlog::info("Could not create Vulkan surface");
    }

    VkDebugUtilsMessengerEXT debugUtilsMessenger{
        createDebugMessenger(instance)};

    m_State = VulkanState{.instance = instance, .surface = surface};
}
void SYN::VulkanBackend::shutdown() {
    vkDestroySurfaceKHR(m_State.instance, m_State.surface, nullptr);
    destroyDebugMessenger(m_State.instance, m_State.debugUtilsMessenger);
    vkDestroyInstance(m_State.instance, nullptr);
}
