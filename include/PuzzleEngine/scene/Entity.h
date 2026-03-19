#pragma once
#include <entt/entity/entity.hpp>
#include "scene/SceneState.h"
#include "PuzzleEngine/core/InputTypes.h"

namespace SYN {
    class Scene;

    //entity is just a convenient wrapper around entt stuff, makes the api more accessible
    class Entity {
    public:
        explicit Entity(SceneState *sceneState, entt::entity handle);

        explicit Entity(entt::registry *registry, entt::entity handle);

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
        void removeComponent() {
            m_Registry->remove<T>(m_Handle);
        }

        template<typename T>
        bool hasComponent() {
            return m_Registry->any_of<T>(m_Handle);
        }

        //I want to avoid operator magic where possible so I think we should just directly call get methods if we need to
        entt::entity getHandle() { return m_Handle; }

    private:
        entt::registry *m_Registry;
        entt::entity m_Handle;
    };
}
