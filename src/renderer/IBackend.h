#pragma once

namespace SYN {
class Window;

class IBackend {
  public:
    IBackend() = default;
    virtual ~IBackend() = default;

    virtual void init(Window &window) = 0;
    virtual void render(Window &window) = 0;
    virtual void shutdown() = 0;

  private:
};

} // namespace SYN
