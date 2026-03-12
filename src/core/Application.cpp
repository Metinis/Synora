#include <GLFW/glfw3.h>

#include <PuzzleEngine/core/Application.h>
#include <PuzzleEngine/core/Input.h>
#include <PuzzleEngine/core/Window.h>

using namespace SYN;

Application *Application::s_Instance = nullptr;
Application::Application() { s_Instance = this; }

void Application::init() {
    // create window etc
    m_IsRunning = true;

    m_Window =
        std::make_unique<Window>(Window::Config{"Synora Engine", 1280, 720});

    m_InputManager = std::make_unique<Input>();

    // Could make Input a shared pointer instead
    // of passing raw pointer, but m_WindowData should
    // only be used as long as the Application is running,
    // and Input lifetime is tied to Application runtime.

    m_WindowData = {m_InputManager.get()};

    glfwSetWindowUserPointer(m_Window->getHandle(), &m_WindowData);
    m_InputManager->init(m_Window);
}

void Application::run() {
    // while running and window open
    while (m_IsRunning && m_Window->isRunning()) {
        glfwPollEvents();
    }
}

void Application::shutdown() {}

Application::~Application() = default;
