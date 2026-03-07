#pragma once
#include <string_view>
#include <glfw/glfw3.h>

namespace SYN {
    class Window {
    public:
        Window(std::string_view name, int width, int height);
        bool isRunning() const;
        ~Window();
    private:
        GLFWwindow* m_Window{};
    };
}
