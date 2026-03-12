#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

#include <PuzzleEngine/core/Input.h>
#include <PuzzleEngine/core/Window.h>

struct {
} InputGlobals;

void keyCallback(GLFWwindow *window, int key, int scancode, int action,
                 int mods) {
    struct WindowPointers {
        SYN::Input *input;
    };

    WindowPointers *user_data =
        static_cast<WindowPointers *>(glfwGetWindowUserPointer(window));

    if (user_data == nullptr || user_data->input == nullptr) {
        spdlog::error("Input not passed to window context...");
        return;
    }

    user_data->input->onKeyEvent(key, action);
}

void SYN::Input::onKeyEvent(int32_t keyCode, int32_t action) {
    if (action == GLFW_PRESS && !m_RawKeyStates[keyCode]) {
        m_RawKeyStates[keyCode] = true;
        m_RawInputQueue.emplace_back(RawInputState::Down, keyCode);
    } else if (action == GLFW_RELEASE && m_RawKeyStates[keyCode]) {
        m_RawKeyStates[keyCode] = false;
        m_RawInputQueue.emplace_back(RawInputState::Up, keyCode);
    }
}

void SYN::Input::init(const std::unique_ptr<Window> &window) {
    glfwSetKeyCallback(window->getHandle(), keyCallback);
}
