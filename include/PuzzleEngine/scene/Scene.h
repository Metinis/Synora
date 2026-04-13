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

    bool isValidEntity(Entity entity);

    void init(EngineContext *ctx);
    Entity createEntity(const std::string &tag = "Unnamed Entity");
    void removeEntity(Entity entity);

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
    AssetManager *m_AssetManager;
    SceneState m_SceneState;

    friend class Entity;
};

} // namespace SYN
