#pragma once
#include "Layer.h"
#include "PuzzleEngine/project/Project.h"

namespace SYN {
class Window;
class Input;
class Renderer;
class Scene;

struct EngineContext {
    std::unique_ptr<Window> window;
    std::unique_ptr<Input> inputManager;
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<Scene> scene;
    ProjectConfig projectConfig;
};
class Application {
  public:
    Application();
    void init();
    void run();
    void shutdown();
    static Application &get() { return *s_Instance; }
    virtual ~Application();

    std::unique_ptr<Input> &GetInput();

  private:
    static Application *s_Instance;

    EngineContext m_EngineContext;

    std::deque<ILayer *> m_Layers;

    bool m_IsRunning{};

    int m_WindowWidth;
    int m_WindowHeight;
};
} // namespace SYN
