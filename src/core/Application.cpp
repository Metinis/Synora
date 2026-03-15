#include <GLFW/glfw3.h>

#include <PuzzleEngine/core/Application.h>
#include <PuzzleEngine/core/Input.h>
#include <PuzzleEngine/core/InputContext.h>
#include <PuzzleEngine/core/Window.h>
#include <PuzzleEngine/Scene/Scene.h>
#include "renderer/Renderer.h"

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
    m_Scene = std::make_unique<Scene>();
    m_Layers.push_back(m_Scene.get());
    //push any sort of ui/overlay to front

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

        for (auto& layer : m_Layers) {
            if (layer->isLayerActive)
                layer->onUpdate(m_Window->getDeltaTime());
        }

        for (auto& layer : m_Layers) {
            //could pass renderer stuff here
            if (layer->isLayerActive)
                layer->onRender();
        }
    }
}

void Application::shutdown() { m_Renderer->shutdown(); }

Application::~Application() = default;
