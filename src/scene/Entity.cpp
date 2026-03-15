#include "PuzzleEngine/Scene/Entity.h"

SYN::Entity::Entity(SceneState *sceneState, entt::entity handle) : m_Registry(&sceneState->registry), m_Handle(handle) {}

//or if you wanna be more explicit
SYN::Entity::Entity(entt::registry *registry, entt::entity handle) : m_Registry(registry), m_Handle(handle) {}
