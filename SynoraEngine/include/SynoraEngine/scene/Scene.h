#pragma once
#include "ISystem.h"

#include <entt/entt.hpp>

#include "../renderer/Renderer.h"
#include "SceneState.h"
#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/core/Layer.h"
#include "SynoraEngine/scene/Entity.h"

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
    void onUIRender() override {};
    Entity getEntity(UUID id);
    bool isDescendantOf(Entity parent, Entity possibleChild);

    bool loadAllSystems(const std::filesystem::path& path);

    bool isValidEntity(Entity entity);
    void onUUIDRemoved(entt::registry& reg, entt::entity e); //same as removing entity
    void onMeshAdded(entt::registry& reg, entt::entity e);
    void onMeshRemoved(entt::registry& reg, entt::entity e);
    void onParentAdded(entt::registry& reg, entt::entity e);
    void onParentRemoved(entt::registry& reg, entt::entity e);

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
    std::vector<std::function<void()>> m_OnUpdate;
    std::unordered_map<UUID, Entity> m_EntityUUIDCache;
    std::vector<Entity> m_EntityCache; //used for updating world matrices, create entity will update

    std::vector<std::unique_ptr<ISystem>> m_Systems{};

    friend class Entity;
};

} // namespace SYN
