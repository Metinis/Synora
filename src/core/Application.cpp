#include "PuzzleEngine/project/AssetManager.h"
#include "PuzzleEngine/project/Project.h"
#include "PuzzleEngine/scene/Scene.h"
#include "renderer/Renderer.h"
#include <GLFW/glfw3.h>
#include <PuzzleEngine/core/Application.h>
#include <PuzzleEngine/core/Input.h>
#include <PuzzleEngine/core/InputContext.h>
#include <PuzzleEngine/core/Window.h>

using namespace SYN;

Application *Application::s_Instance = nullptr;
Application::Application() {
    s_Instance = this;

    m_EngineContext.window = std::make_unique<Window>();
    m_EngineContext.inputManager = std::make_unique<Input>();
    m_EngineContext.renderer = std::make_unique<Renderer>();
    m_EngineContext.scene = std::make_unique<Scene>();

    ProjectConfig projectConfig{
        .resourceRoot = "", // todo add root
        .assetManager = std::make_unique<AssetManager>(),
    };
    m_EngineContext.projectConfig = std::move(projectConfig);
}

void Application::init() {
    spdlog::set_level(spdlog::level::debug);
    // create window etc
    m_IsRunning = true;

    m_EngineContext.window->init(Window::Config{"Synora Engine", 1280, 720});

    m_EngineContext.inputManager->init(&m_EngineContext);
    m_EngineContext.renderer->init(&m_EngineContext);
    m_EngineContext.projectConfig.assetManager->init(&m_EngineContext);
    m_EngineContext.scene->init(&m_EngineContext);

    m_Layers.push_back(m_EngineContext.scene.get());
}

void Application::run() {
    // while running and window open
    while (m_IsRunning && m_EngineContext.window->isRunning()) {
        glfwPollEvents();

        for (auto &l : m_Layers) {
            l->onUpdate(m_EngineContext.window->getDeltaTime());
        }
        for (auto &l : m_Layers) {
            l->onRender();
        }
        for (auto &l : m_Layers) {
            // ui render
        }

        m_EngineContext.inputManager->processInputQueue();

        m_EngineContext.renderer->render(*m_EngineContext.window.get());
    }
}

void Application::shutdown() { m_EngineContext.renderer->shutdown(); }

std::unique_ptr<Input> &Application::GetInput() {
    return m_EngineContext.inputManager;
}

Application::~Application() = default;
