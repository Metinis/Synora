#pragma once
#include <entt/entity/entity.hpp>

#include "Components.h"
#include "SynoraEngine/scene/SceneState.h"
#include "SynoraEngine/core/InputTypes.h"
#include "SynoraEngine/project/UUID.h"

namespace SYN {
    class Scene;

    //entity is just a convenient wrapper around entt stuff, makes the api more accessible
    class Entity {
    public:
        explicit Entity() = default;

        explicit Entity(SceneState *sceneState, entt::entity handle);

        explicit Entity(entt::registry *registry, entt::entity handle);

        void addParent(const ParentComp &pc); //TODO when adding parent, preserve transforms
        //ParentComp& addParent(UUID parentID);

        //void removeParent();
        void addRuntimeComponent(const std::string& compName);
        void removeRuntimeComponent(const std::string& compName);

        template<typename T>
        T &getComponent() {
            return m_Registry->get<T>(m_Handle);
        }


        template<typename T, typename... Args>
        T &addComponent(Args... args) {
            return m_Registry->emplace<T>(m_Handle,
                                          std::forward<Args>(args)...);
        }

        template<typename T, typename... Args>
        T &replaceComponent(Args... args) {
            return m_Registry->replace<T>(m_Handle,
                                          std::forward<Args>(args)...);
        }

        template<typename T>
        T *tryGetComponent() {
            return m_Registry->try_get<T>(m_Handle);
        }

        template<typename T>
        void removeComponent() const {
            m_Registry->remove<T>(m_Handle);
        }

        template<typename T>
        bool hasComponent() {
            return m_Registry->any_of<T>(m_Handle);
        }

        template<typename T>
        bool hasComponent() const {
            return m_Registry->any_of<T>(m_Handle);
        }

        UUID getUUID() const {
            assert(hasComponent<UUIDComp>());
            return m_Registry->get<UUIDComp>(m_Handle).id;
        }

        bool isValid() const {
            return m_Registry != nullptr && m_Handle != entt::null;
        }
        //I want to avoid operator magic where possible so I think we should just directly call get methods if we need to
        [[nodiscard]] entt::entity getHandle() const { return m_Handle; }
        bool operator==(const Entity& other) const noexcept
        {
            return getHandle() == other.getHandle();
        }

    private:
        entt::registry *m_Registry{};
        entt::entity m_Handle{};
    };
}
namespace std {
template<>
struct hash<SYN::Entity>
{
    size_t operator()(const SYN::Entity& e) const noexcept
    {
        return hash<uint32_t>()((uint32_t)e.getHandle());
    }
};
}
