#pragma once
#include <entt/entt.hpp>

#include "PuzzleEngine/core/Application.h"
#include "PuzzleEngine/core/Layer.h"
#include "PuzzleEngine/scene/Entity.h"
#include "renderer/Renderer.h"
#include "scene/SceneState.h"

namespace SYN {
class Renderer;
class AssetManager;

// probably split scene into scene manager and scene contain like this
class Scene : public ILayer {
  public:
    Scene();
    ~Scene() override = default;

    void onUpdate(float dt) override;
    void onAttach() override;
    void onDettach() override;
    void onRender() override;
    void onUIRender() override;

    void init(EngineContext *ctx);
    Entity createEntity();

    template <typename T> std::vector<Entity> getEntities() {
        std::vector<Entity> ret;
        for (auto &e : m_SceneState.registry.view<T>()) {
            ret.push_back(Entity(&m_SceneState, e));
        }
        return ret;
    }

  private:
    Renderer *m_Renderer;
    Window *m_Window;
    SceneState m_SceneState;
    Camera m_Camera;

    float m_Dx;
    float m_Dy;
    float m_Dz;
    float m_DxRot;
    float m_DyRot;
    bool m_CursorHidden{};
    bool m_LastCursorHiddenState{}; // used to prevent camera from jumping on
                                    // hitting esc

    float m_WalkSpeed{1.f};
    float m_RunMultiplier{2.f};
    bool m_IsRunning{};

    friend class Entity;
};

} // namespace SYN
