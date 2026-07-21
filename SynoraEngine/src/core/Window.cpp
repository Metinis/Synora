#include <GLFW/glfw3.h>

#include <SynoraEngine/core/Window.h>

#include <spdlog/spdlog.h>

SYN::Window::Window() {}

void SYN::Window::init(const WindowConfig &config) {
    if (!glfwInit())
        spdlog::error("Failed to initialize GLFW!");

    if (config.openGLConfig.has_value()) {
        OpenGLConfig settings = config.openGLConfig.value();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, settings.versionMajor);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, settings.versionMinor);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);
    } else {
        // no api if vulkan
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    }

    m_Window = glfwCreateWindow(config.width, config.height,
                                config.title.data(), nullptr, nullptr);

    if (!m_Window) {
        spdlog::error("Failed to create window!");
    }
}

bool SYN::Window::isRunning() const { return !glfwWindowShouldClose(m_Window); }

void SYN::Window::calculateDeltaTime() {
    auto currentTime = static_cast<float>(glfwGetTime());
    m_DeltaTime = currentTime - m_LastTime;
    m_LastTime = currentTime;
}
float SYN::Window::getDeltaTime() const { return m_DeltaTime; }

SYN::Window::~Window() { glfwDestroyWindow(m_Window); }

struct GLFWwindow *SYN::Window::getHandle() { return m_Window; }

void SYN::Window::disableCursor() {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}
void SYN::Window::enableCursor() {
    glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

std::tuple<uint32_t, uint32_t> SYN::Window::getScreenSize() const {
    int w, h;
    glfwGetWindowSize(m_Window, &w, &h);
    return std::make_tuple(w, h);
}
