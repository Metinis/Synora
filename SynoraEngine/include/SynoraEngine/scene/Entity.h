#pragma once
#include <entt/entity/entity.hpp>
#include <entt/entity/registry.hpp>

#include <SynoraEngine/project/UUID.h>

namespace SYN {
// entity is just a convenient wrapper around entt stuff, makes the api more
// accessible
class Entity {
  public:
    Entity() = default;

    void setParent(Entity parent, bool preserveWorldTransform = true);
    void removeParent(bool preserveWorldTransform = true);
    bool isDescendantOf(Entity other) const;
    UUID getUUID() const;

    template <typename T> const T &getComponent() const {
        return m_Registry->get<T>(m_Handle);
    }

    template <typename T> T &getComponent() {
        return m_Registry->get<T>(m_Handle);
    }

    template <typename T, typename... Args> T &addComponent(Args &&...args) {
        return m_Registry->emplace<T>(m_Handle, std::forward<Args>(args)...);
    }

    template <typename T, typename... Args>
    T &replaceComponent(Args &&...args) {
        return m_Registry->replace<T>(m_Handle, std::forward<Args>(args)...);
    }

    template <typename T> const T *tryGetComponent() const {
        return m_Registry->try_get<T>(m_Handle);
    }

    template <typename T> T *tryGetComponent() {
        return m_Registry->try_get<T>(m_Handle);
    }

    template <typename T> void removeComponent() {
        m_Registry->remove<T>(m_Handle);
    }

    template <typename T> bool hasComponent() const {
        return m_Registry->any_of<T>(m_Handle);
    }

    bool isValid() const;

  private:
    friend class Scene;

    explicit Entity(class Scene *sceneOwner, entt::registry *registry,
                    entt::entity handle);

    entt::entity getHandle() const { return m_Handle; }

  private:
    entt::registry *m_Registry = nullptr;
    entt::entity m_Handle = entt::null;
    class Scene *m_SceneOwner = nullptr;
};
} // namespace SYN
