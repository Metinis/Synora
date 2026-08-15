#include "SynoraEngine/renderer/Renderer.h"
#include "SynoraEngine/project/AssetManager.h"
#include "SynoraEngine/project/Project.h"
#include "SynoraEngine/scene/Scene.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "scene/CameraSystem.h"
#include <GLFW/glfw3.h>
#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/scene/RuntimeCompManager.h>
#include "SynoraEngine/scene/ScriptManager.h"
#include "tinyfiledialogs.h"
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
  m_EngineContext.scriptManager = std::make_unique<ScriptManager>();
  m_EngineContext.fileWatcher = std::make_unique<efsw::FileWatcher>();
  m_EngineContext.compManager = std::make_unique<RuntimeCompManager>();
  m_EngineContext.assetManager = std::make_unique<AssetManager>();

  ProjectConfig projectConfig{
    .projectRoot = "",
    .resourceRoot = "",
  };
  m_EngineContext.projectConfig = std::move(projectConfig);
}
static void selectProject(ProjectConfig& projectConfig, const std::filesystem::path& projectPath) {
  if(projectPath != "") {
    assert(std::filesystem::exists(projectPath / "assets"));
    projectConfig.resourceRoot = projectPath;
  }

  std::string currentDir = std::filesystem::current_path().string();
  std::filesystem::path gamePath;
  if(projectPath == "") {
    const char *folder = tinyfd_selectFolderDialog("Select Game Folder", currentDir.c_str());
    gamePath = std::filesystem::path(folder);
  } else {
    gamePath = projectPath;
  }

  if (!exists(gamePath / "systems")) {
    std::filesystem::create_directory(gamePath / "systems");
    spdlog::debug("Systems dir not found.. Creating a new one");
  }
  if (!exists(gamePath / "assets")) {
    std::filesystem::create_directory(gamePath / "assets");
    spdlog::debug("Assets dir not found.. Creating a new one");
  }
  projectConfig.projectRoot = gamePath;
  projectConfig.resourceRoot = gamePath / "assets";

}
void Application::init(const std::filesystem::path& projectPath) {
  spdlog::set_level(spdlog::level::debug);

  // create window etc
  m_IsRunning = true;
  selectProject(m_EngineContext.projectConfig, projectPath);

  m_EngineContext.window->init(Window::Config{"Synora Engine", 1280, 720});

  m_EngineContext.inputManager->init(&m_EngineContext);
  m_EngineContext.renderer->init(&m_EngineContext);
  m_EngineContext.assetManager->init(&m_EngineContext);
  m_EngineContext.scene->init(&m_EngineContext);
  m_EngineContext.scriptManager->init(&m_EngineContext);
  m_EngineContext.cameraSystem->init(&m_EngineContext);
  m_EngineContext.fileWatcher->watch();

  m_Layers.push_back(m_EngineContext.scene.get());
  m_Layers.push_back(m_EngineContext.cameraSystem.get());
  m_Layers.push_back(m_EngineContext.scriptManager.get());
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
    m_EngineContext.assetManager->flushRefCount();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    for (auto &l : m_Layers) {
      l->onUIRender();
    }
    ImGui::Render();
    ImGui::EndFrame();

    m_EngineContext.renderer->render(*m_EngineContext.window.get());
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
