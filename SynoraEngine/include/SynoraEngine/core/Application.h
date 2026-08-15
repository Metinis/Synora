#pragma once
#include "Layer.h"
#include "SynoraEngine/project/Project.h"
#include "efsw/efsw.hpp"
#define GLM_ENABLE_EXPERIMENTAL


namespace SYN {
class RuntimeCompManager;
class Window;
class Input;
class Renderer;
class Scene;
class CameraSystem;
class ScriptManager;
class AssetManager;

struct EngineContext {
    std::unique_ptr<Window> window;
    std::unique_ptr<Input> inputManager;
    std::unique_ptr<AssetManager> assetManager;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<CameraSystem> cameraSystem;
    std::unique_ptr<ScriptManager> scriptManager;
    std::unique_ptr<efsw::FileWatcher> fileWatcher;
    std::unique_ptr<RuntimeCompManager> compManager;
    ProjectConfig projectConfig;
    bool isGameRunning {false};
};
class Application {
  public:
    Application();
    virtual void init(const std::filesystem::path& projectPath = "");
    void run();
    void shutdown();
    static Application &get() { return *s_Instance; }
    EngineContext* getCtx() { return &m_EngineContext; }
    virtual ~Application();

    std::unique_ptr<Input> &GetInput();

  protected:
    static Application *s_Instance;

    EngineContext m_EngineContext;

    std::deque<ILayer *> m_Layers;

    bool m_IsRunning{};

    int m_WindowWidth;
    int m_WindowHeight;
};
} // namespace SYN
