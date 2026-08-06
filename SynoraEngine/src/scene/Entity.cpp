#include "SynoraEngine/scene/Entity.h"

#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/scene/Scene.h"

using namespace SYN;
Entity::Entity(SceneState *sceneState, entt::entity handle)
    : m_SceneState(sceneState), m_Handle(handle) {}

void Entity::addParent(const ParentComp &pc) {
    addComponent<ParentComp>(pc);
}
void Entity::addRuntimeComponent(const std::string &compName) {
    Application::get().getCtx()->scene->addRuntimeComponent(*this, compName);
}
void Entity::removeRuntimeComponent(const std::string &compName) {
    Application::get().getCtx()->scene->removeRuntimeComponent(*this, compName);
}
