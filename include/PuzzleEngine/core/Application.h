#pragma once
#include "Layer.h"
#include "PuzzleEngine/project/Project.h"

namespace SYN {
class Window;
class Input;
class Renderer;
class Scene;

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
    std::unique_ptr<Window> m_Window;
    std::unique_ptr<Input> m_InputManager;
    std::unique_ptr<Renderer> m_Renderer;
    std::unique_ptr<Scene> m_Scene;
    ProjectConfig m_ProjectConfig;

    std::deque<ILayer*> m_Layers;

    bool m_IsRunning{};

    int m_WindowWidth;
    int m_WindowHeight;

  private:
    struct WindowPointers {
        Input *m_Input;
    };
    WindowPointers m_WindowData;
};
} // namespace SYN
