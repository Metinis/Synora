#pragma once
#include "entt/entity/registry.hpp"

//here to avoid circular dependency with entity
namespace SYN {
    struct SceneState {
        entt::registry registry;
    };
}