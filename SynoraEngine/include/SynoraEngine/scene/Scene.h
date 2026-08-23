#pragma once

#include "SynoraEngine/project/UUID.h"
#include "SynoraEngine/scene/Entity.h"

#include <glm/mat4x4.hpp>

namespace SYN {

class Scene {
  public:
    Scene() = default;
    ~Scene();

    Scene(std::string_view sceneName);
    Scene(const Scene &) = delete;
    Scene(Scene &&) = delete;

    Entity createEntity(std::string_view tag);
    void removeEntity(Entity entity);

    Entity getEntity(UUID id);

    glm::mat4 getWorldTransformOf(Entity entity);

    const std::string &getName() const;

    template <typename... Components> std::vector<Entity> getEntities() {
        std::vector<Entity> entities;
        for (entt::entity e : m_Registry.view<Components...>()) {
            entities.emplace_back(Entity(this, &m_Registry, e));
        }
        return entities;
    }

    template <typename... Components, typename Fn> void forEach(Fn &&fn) {
        auto view = m_Registry.view<Components...>();
        for (auto entity : view) {
            fn(Entity(this, &m_Registry, entity),
               view.template get<Components>(entity)...);
        }
    }

  private:
    std::string m_Name;
    entt::registry m_Registry;
    std::unordered_map<UUID, Entity> m_UUIDToEntity;
};

} // namespace SYN
