#include <GLFW/glfw3.h>

#include <PuzzleEngine/core/Window.h>

#include <spdlog/spdlog.h>

SYN::Window::Window(const Config &config) {
    if (!glfwInit())
        spdlog::error("Failed to initialize GLFW!");

    // no api if vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_Window = glfwCreateWindow(config.width, config.height,
                                config.title.data(), nullptr, nullptr);

    if (!m_Window) {
        spdlog::error("Failed to create window!");
    }
}

bool SYN::Window::isRunning() const { return !glfwWindowShouldClose(m_Window); }

SYN::Window::~Window() { glfwDestroyWindow(m_Window); }

struct GLFWwindow *SYN::Window::getHandle() { return m_Window; }
