#include <GLFW/glfw3.h>
#include <PuzzleEngine/core/Application.h>
#include <PuzzleEngine/core/Input.h>
#include <PuzzleEngine/core/InputContext.h>
#include <PuzzleEngine/core/Window.h>
#include "PuzzleEngine/project/AssetManager.h"
#include "PuzzleEngine/scene/Scene.h"
#include "renderer/Renderer.h"
#include "PuzzleEngine/project/Project.h"

using namespace SYN;

Application *Application::s_Instance = nullptr;
Application::Application() {
    s_Instance = this;

    m_Window = std::make_unique<Window>();
    m_InputManager = std::make_unique<Input>();
    m_Renderer = std::make_unique<Renderer>();
    m_Scene = std::make_unique<Scene>();

    ProjectConfig projectConfig {
        .resourceRoot = "", //todo add root
        .assetManager = std::make_unique<AssetManager>(),
    };
    m_ProjectConfig = std::move(projectConfig);
}

void Application::init() {
    spdlog::set_level(spdlog::level::debug);
    // create window etc
    m_IsRunning = true;

    // Could make Input a shared pointer instead
    // of passing raw pointer, but m_WindowData should
    // only be used as long as the Application is running,
    // and Input lifetime is tied to Application runtime.
    m_WindowData = {m_InputManager.get()};

    glfwSetWindowUserPointer(m_Window->getHandle(), &m_WindowData);

    m_Window->init(Window::Config{"Synora Engine", 1280, 720});
    m_InputManager->init(m_Window);
    m_Renderer->init(*m_Window);
    m_ProjectConfig.assetManager->init(m_Renderer.get());
    m_Scene->init(m_Renderer.get(), m_ProjectConfig.assetManager.get());

    m_Layers.push_back(m_Scene.get());
}

void Application::run() {
    // while running and window open
    while (m_IsRunning && m_Window->isRunning()) {
        glfwPollEvents();

        for (auto& l : m_Layers) {
            l->onUpdate(m_Window->getDeltaTime());
        }
        for (auto& l : m_Layers) {
            l->onRender();
        }
        for (auto& l : m_Layers) {
            //ui render
        }

        m_InputManager->processInputQueue();

        m_Renderer->render(*m_Window.get());
    }
}

void Application::shutdown() { m_Renderer->shutdown(); }

std::unique_ptr<Input> &Application::GetInput() { return m_InputManager; }

Application::~Application() = default;
