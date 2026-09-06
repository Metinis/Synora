#include "SynoraEngine/scene/Entity.h"
#include "SynoraEngine/scene/Scene.h"

#include <SynoraEngine/scene/components/ParentComponent.h>
#include <SynoraEngine/scene/components/TransformComponent.h>
#include <SynoraEngine/scene/components/UUIDComponent.h>

namespace SYN {
Entity::Entity(Scene *sceneOwner, entt::registry *registry, entt::entity handle)
    : m_SceneOwner(sceneOwner), m_Registry(registry), m_Handle(handle) {}

bool Entity::isValid() const {
    return m_Registry != nullptr && m_Handle != entt::null &&
           m_SceneOwner != nullptr && m_Registry->valid(m_Handle);
}

void Entity::setParent(Entity parent, bool preserveWorldTransform) {
    if (!parent.isValid())
        return;

    if (parent.isDescendantOf(*this))
        return;

    glm::mat4 childTransform = m_SceneOwner->getWorldTransformOf(*this);
    glm::mat4 parentTransform = m_SceneOwner->getWorldTransformOf(parent);

    if (hasComponent<ParentComponent>())
        removeComponent<ParentComponent>();

    if (preserveWorldTransform) {
        getComponent<TransformComponent>().setLocalMatrix(
            glm::inverse(parentTransform) * childTransform);
    }

    addComponent<ParentComponent>(parent.getUUID());
}

UUID Entity::getUUID() const { return getComponent<UUIDComponent>().id; }

void Entity::removeParent(bool preserveWorldTransform) {
    if (preserveWorldTransform && hasComponent<TransformComponent>() &&
        hasComponent<ParentComponent>()) {
        auto &tc = getComponent<TransformComponent>();
        auto &pc = getComponent<ParentComponent>();
        auto parentE = m_SceneOwner->getEntity(pc.id);

        if (parentE.isValid() && parentE.hasComponent<TransformComponent>()) {
            tc.setLocalMatrix(m_SceneOwner->getWorldTransformOf(*this));
        }
    }
    removeComponent<ParentComponent>();
}

bool Entity::isDescendantOf(Entity other) const {
    if (!other.isValid())
        return false;

    if (!isValid())
        return false;

    if (!hasComponent<ParentComponent>())
        return false;

    auto current = *this;
    while (current.hasComponent<ParentComponent>()) {
        auto pid = current.getComponent<ParentComponent>().id;
        if (pid == other.getComponent<UUIDComponent>().id)
            return true;

        current = m_SceneOwner->getEntity(pid);
        if (!current.isValid())
            break;
    }
    return false;
}

} // namespace SYN
