#pragma once
#include <entt/entt.hpp>
#include "PuzzleEngine/Scene/Entity.h"
#include "scene/SceneState.h"

namespace SYN {
    class Entity;

    class Scene {
    public:
        Scene();
        ~Scene() = default;
        void init();
        Entity createEntity();

        template<typename T>
        std::vector<Entity> getEntities() {
            std::vector<Entity> ret;
            for (auto& e : m_SceneState.registry.view<T>()) {
                ret.push_back(Entity(this, e));
            }
            return ret;
        }
    private:
        SceneState m_SceneState;
        friend class Entity;
    };
}
