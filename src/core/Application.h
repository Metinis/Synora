#pragma once
#include <memory>

namespace SYN {
    class Window;

    class Application {
    public:
        Application();
        void init();
        void run();
        void shutdown();
        static Application& get() { return *s_Instance; }
        virtual ~Application();
    private:
        static Application* s_Instance;
        std::unique_ptr<Window> m_Window;
        bool m_IsRunning{};
    };
}
