#include "SynoraEngine/scene/Scene.h"

#include "SynoraEngine/scene/components/ParentComponent.h"
#include "SynoraEngine/scene/components/TagComponent.h"
#include "SynoraEngine/scene/components/TransformComponent.h"
#include "SynoraEngine/scene/components/UUIDComponent.h"

using namespace SYN;

Scene::Scene(std::string_view sceneName) : m_Name(sceneName) {}
Scene::~Scene() {
    m_Registry.clear();
    m_UUIDToEntity.clear();
    m_Name.clear();
}

const std::string &Scene::getName() const { return m_Name; }

Entity Scene::getEntity(UUID id) {
    if (auto entityIt = m_UUIDToEntity.find(id);
        entityIt != m_UUIDToEntity.cend()) {
        return entityIt->second;
    }
    return Entity{};
}

glm::mat4 Scene::getWorldTransformOf(Entity entity) {
    if (!entity.isValid())
        return glm::mat4(1.0f);

    const TransformComponent *current =
        entity.tryGetComponent<TransformComponent>();

    if (!current)
        return glm::mat4(1.0f);

    glm::mat4 parent = glm::mat4(1.0f);
    ParentComponent *parentComponent =
        entity.tryGetComponent<ParentComponent>();
    if (parentComponent) {
        parent = getWorldTransformOf(getEntity(parentComponent->id));
    }
    return parent * current->getLocalMatrix();
}

Entity Scene::createEntity(std::string_view tag) {
    entt::entity newEntityHandle = m_Registry.create();
    auto ent = Entity(this, &m_Registry, newEntityHandle);
    auto id = generateUUID();
    ent.addComponent<UUIDComponent>(id);
    ent.addComponent<TagComponent>(TagComponent{.tag = std::string(tag)});
    ent.addComponent<TransformComponent>();
    m_UUIDToEntity[id] = ent;
    return ent;
}

void Scene::removeEntity(Entity entity) {
    if (!entity.isValid())
        return;

    m_UUIDToEntity.erase(entity.getUUID());

    std::vector<Entity> childrenToRemove;
    for (auto &ent : getEntities<ParentComponent>()) {
        if (ent.isValid() &&
            ent.getComponent<ParentComponent>().id == entity.getUUID()) {
            childrenToRemove.push_back(ent);
        }
    }

    for (auto &child : childrenToRemove) {
        removeEntity(child);
    }

    m_Registry.destroy(entity.getHandle());
}
