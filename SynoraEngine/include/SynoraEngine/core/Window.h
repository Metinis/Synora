#pragma once

#include "Application.h"

namespace SYN {
class Window {
  public:
  public:
    Window();
    void init(const WindowConfig &config);
    bool isRunning() const;
    ~Window();

    struct GLFWwindow *getHandle();
    void calculateDeltaTime();
    float getDeltaTime() const;

    void disableCursor();
    void enableCursor();

    std::tuple<uint32_t, uint32_t> getScreenSize() const;

  private:
    struct GLFWwindow *m_Window{};
    float m_LastTime;
    float m_DeltaTime;
};
} // namespace SYN
