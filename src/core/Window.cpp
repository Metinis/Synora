#include "Window.h"

#include "spdlog/spdlog.h"

SYN::Window::Window(std::string_view name, int width, int height) {
    if (!glfwInit())
        spdlog::error("Failed to initialize GLFW!");

    m_Window = glfwCreateWindow(width, height, name.data(), nullptr, nullptr);

    //no api if vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    if (!m_Window) {
        spdlog::error("Failed to create window!");
    }
}

bool SYN::Window::isRunning() const {
    return !glfwWindowShouldClose(m_Window);
}

SYN::Window::~Window() = default;
