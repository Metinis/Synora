#include "../../include/SynoraEngine/renderer/Renderer.h"
#include "SynoraEngine/project/AssetManager.h"
#include "SynoraEngine/project/Project.h"
#include "SynoraEngine/scene/Scene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_vulkan.h"
#include "scene/CameraSystem.h"
#include <GLFW/glfw3.h>
#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/core/Input.h>
#include <SynoraEngine/core/InputContext.h>
#include <SynoraEngine/core/Window.h>

using namespace SYN;

Application *Application::s_Instance = nullptr;
Application::Application() {
    s_Instance = this;

    m_EngineContext.window = std::make_unique<Window>();
    m_EngineContext.inputManager = std::make_unique<Input>();
    m_EngineContext.renderer = std::make_unique<Renderer>();
    m_EngineContext.scene = std::make_unique<Scene>();
    m_EngineContext.cameraSystem = std::make_unique<CameraSystem>();

    ProjectConfig projectConfig{
        .resourceRoot = "", // todo add root
        .assetManager = std::make_unique<AssetManager>(),
    };
    m_EngineContext.projectConfig = std::move(projectConfig);
    m_EngineContext.windowConfig = WindowConfig{"Synora Engine", 1280, 720};
}

void Application::init() {
    spdlog::set_level(spdlog::level::debug);
    // create window etc
    m_IsRunning = true;

    m_EngineContext.window->init(m_EngineContext.windowConfig);

    m_EngineContext.inputManager->init(&m_EngineContext);
    m_EngineContext.renderer->init(&m_EngineContext);
    m_EngineContext.projectConfig.assetManager->init(&m_EngineContext);
    m_EngineContext.scene->init(&m_EngineContext);
    m_EngineContext.cameraSystem->init(&m_EngineContext);

    m_Layers.push_back(m_EngineContext.scene.get());
    m_Layers.push_back(m_EngineContext.cameraSystem.get());
}

void Application::run() {
    // while running and window open
    double lastTime{glfwGetTime()};
    while (m_IsRunning && m_EngineContext.window->isRunning()) {
        glfwPollEvents();
        m_EngineContext.inputManager->processInputQueue();

        double currentTime{glfwGetTime()};
        float dt{static_cast<float>(currentTime - lastTime)};
        lastTime = currentTime;
        for (auto &l : m_Layers) {
            l->onUpdate(dt);
        }

        if (m_EngineContext.windowConfig.openGLConfig.has_value()) {
            ImGui_ImplOpenGL3_NewFrame();
        } else {
            ImGui_ImplVulkan_NewFrame();
        }

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        for (auto &l : m_Layers) {
            l->onUIRender();
        }
        ImGui::Render();
        ImGui::EndFrame();

        if (!m_EngineContext.windowConfig.openGLConfig.has_value()) {
            m_EngineContext.renderer->render(*m_EngineContext.window.get());
        }

        for (auto &l : m_Layers) {
            l->onRender();
        }
    }
}

void Application::shutdown() { m_EngineContext.renderer->shutdown(); }

std::unique_ptr<Input> &Application::GetInput() {
    return m_EngineContext.inputManager;
}

Application::~Application() = default;
