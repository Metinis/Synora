#pragma once

namespace SYN {
class ILayer {
  public:
    virtual ~ILayer() = default;
    virtual void onAttach() = 0;
    virtual void onUpdate(float dt) = 0;
    virtual void onRender() = 0;
    virtual void onUIRender() = 0;
    virtual void onDetach() = 0;
    bool isLayerActive{true};
};
} // namespace SYN
