#pragma once

#include <SynoraEngine/core/ILayer.h>

namespace SYN {
class AnimationPlayerSystem : public ILayer {
  public:
    ~AnimationPlayerSystem() = default;

    void init(class EngineContext *context);

    void onAttach() override;
    void onUpdate(float dt) override;
    void onRender() override;
    void onUIRender() override;
    void onDettach() override;

  private:
    class SceneManager *m_SceneManager;
};
} // namespace SYN
