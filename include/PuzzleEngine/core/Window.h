#pragma once
#include <string_view>

namespace SYN {
class Window {
  public:
    struct Config {
        std::string_view m_Title;
        uint16_t m_Width;
        uint16_t m_Height;
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
