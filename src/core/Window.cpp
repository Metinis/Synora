#include <GLFW/glfw3.h>

#include <PuzzleEngine/core/Window.h>

#include <spdlog/spdlog.h>

SYN::Window::Window(const Config &config) {
    if (!glfwInit())
        spdlog::error("Failed to initialize GLFW!");

    m_Window = glfwCreateWindow(config.m_Width, config.m_Height,
                                config.m_Title.data(), nullptr, nullptr);

    // no api if vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (!m_Window) {
        spdlog::error("Failed to create window!");
    }
}

bool SYN::Window::isRunning() const { return !glfwWindowShouldClose(m_Window); }

SYN::Window::~Window() { glfwDestroyWindow(m_Window); }
