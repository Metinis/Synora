#include "SynoraEngine/scene/Entity.h"

using namespace SYN;
Entity::Entity(SceneState *sceneState, entt::entity handle)
    : m_Registry(&sceneState->registry), m_Handle(handle) {}

// or if you wanna be more explicit
Entity::Entity(entt::registry *registry, entt::entity handle)
    : m_Registry(registry), m_Handle(handle) {}

void Entity::addParent(const ParentComp &pc) {
    addComponent<ParentComp>(pc);
}

/*ParentComp& Entity::addParent(UUID parentID) {
    if (hasComponent<TransformComp>()) {
        auto& tc = getComponent<TransformComp>();
        auto parentE = m_Scene->getEntity(parentID);

        if (parentE.isValid() && parentE.hasComponent<TransformComp>()) {
            auto& ptc = parentE.getComponent<TransformComp>();
            tc.localPosition =
                glm::inverse(ptc.worldMatrix) *
                glm::vec4(tc.localPosition, 1.0f);
        }
    }
    return m_Registry->emplace<ParentComp>(m_Handle, ParentComp{parentID});
}*/

/*void Entity::removeParent() {
    if (hasComponent<TransformComp>() && hasComponent<ParentComp>()) {
        auto& tc = getComponent<TransformComp>();
        auto& pc = getComponent<ParentComp>();
        auto parentE = m_Scene->getEntity(pc.parentID);

        if (parentE.isValid() && parentE.hasComponent<TransformComp>()) {
            auto& ptc = parentE.getComponent<TransformComp>();
            tc.localPosition =
                ptc.worldMatrix *
                glm::vec4(tc.localPosition, 1.0f);
        }
    }
    m_Registry->remove<ParentComp>(m_Handle);
}*/
