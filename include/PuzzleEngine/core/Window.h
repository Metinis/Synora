#pragma once
#include <string_view>

namespace SYN {
class Window {
  public:
    struct Config {
        std::string_view title;
        uint16_t width;
        uint16_t height;
    };

  public:
    Window(const Config &config);
    bool isRunning() const;
    ~Window();

    struct GLFWwindow *getHandle();

  private:
    struct GLFWwindow *m_Window{};
};
} // namespace SYN
