#pragma once
#include <string_view>

#include "Application.h"

namespace SYN {
class Window {
  public:
    struct Config {
        std::string_view title;
        uint16_t width;
        uint16_t height;
    };

  public:
    Window();
    void init(const Config &config);
    bool isRunning() const;
    ~Window();

    struct GLFWwindow *getHandle();
    void calculateDeltaTime();
    float getDeltaTime() const;

  private:
    struct GLFWwindow *m_Window{};
    float m_LastTime;
    float m_DeltaTime;
};
} // namespace SYN
