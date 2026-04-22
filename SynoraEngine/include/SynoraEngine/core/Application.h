#pragma once
#include "Layer.h"
#include "SynoraEngine/project/Project.h"



namespace SYN {
class Window;
class Input;
class Renderer;
class Scene;
class CameraSystem;



struct EngineContext {
    std::unique_ptr<Window> window;
    std::unique_ptr<Input> inputManager;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Scene> scene;
    std::unique_ptr<CameraSystem> cameraSystem;
    ProjectConfig projectConfig;
};
class Application {
  public:
    Application();
    virtual void init();
    void run();
    void shutdown();
    static Application &get() { return *s_Instance; }
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
