#include <GLFW/glfw3.h>

#include <PuzzleEngine/core/Application.h>
#include <PuzzleEngine/core/Input.h>
#include <PuzzleEngine/core/InputContext.h>
#include <PuzzleEngine/core/Window.h>

#include "../renderer/Renderer.h"

using namespace SYN;

Application *Application::s_Instance = nullptr;
Application::Application() { s_Instance = this; }

void Application::init() {
    // create window etc
    m_IsRunning = true;

    m_Window =
        std::make_unique<Window>(Window::Config{"Synora Engine", 1280, 720});

    m_InputManager = std::make_unique<Input>();
    m_Renderer = std::make_unique<Renderer>();

    // Could make Input a shared pointer instead
    // of passing raw pointer, but m_WindowData should
    // only be used as long as the Application is running,
    // and Input lifetime is tied to Application runtime.

    m_WindowData = {m_InputManager.get()};

    glfwSetWindowUserPointer(m_Window->getHandle(), &m_WindowData);
    m_InputManager->init(m_Window);
    m_Renderer->init(*m_Window);
}

void Application::run() {
    // while running and window open
    while (m_IsRunning && m_Window->isRunning()) {
        glfwPollEvents();

        m_InputManager->processInputQueue();

        m_Renderer->render(*m_Window.get());
    }
}

void Application::shutdown() { m_Renderer->shutdown(); }

std::unique_ptr<Input> &Application::GetInput() { return m_InputManager; }

Application::~Application() = default;
