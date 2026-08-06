#pragma once
#include "RuntimeComp.h"
#include "entt/entity/registry.hpp"

//here to avoid circular dependency with entity
namespace SYN {
    struct SceneState {
        RuntimeCompManager *m_RuntimeCompManager;
        std::unordered_map<entt::entity, std::unordered_map<std::string, RuntimeComponent>> m_RuntimeCompsMap{}; //look up is by comp name
        entt::registry registry{};
    };
}