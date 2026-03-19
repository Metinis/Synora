#pragma once
#include "IBackend.h"

namespace SYN {
class Window;

class Renderer {
  public:
    Renderer() = default;
    ~Renderer() = default;

    void init(Window &window);
    void render(Window &window);
    void shutdown();
    [[nodiscard]] IBackend* getBackend() const {return m_Backend.get();}

  private:
    std::unique_ptr<IBackend> m_Backend;
};

} // namespace SYN
