#pragma once
#include "IBackend.h"

namespace SYN {
class Window;

class Renderer {
  public:
    Renderer() = default;
    ~Renderer() = default;

    void init();
    void shutdown();

  private:
    std::unique_ptr<IBackend> m_Backend;
};

} // namespace SYN
