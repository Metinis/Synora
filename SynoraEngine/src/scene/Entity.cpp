#include "SynoraEngine/scene/Entity.h"

#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/scene/Scene.h"

using namespace SYN;
Entity::Entity(SceneState *sceneState, entt::entity handle)
    : m_SceneState(sceneState), m_Handle(handle) {}

void Entity::addParent(const ParentComp &pc) {
    addComponent<ParentComp>(pc);
}
static RuntimeComponent createRuntimeComponent(const CompDesc &compDesc) {
    RuntimeComponent ret{};
    ret.data.resize(compDesc.size);
    ret.description = compDesc;
    return ret;
}
void Entity::addRuntimeComponent(const std::string& compName) {
    m_SceneState->m_RuntimeCompsMap[getHandle()][compName] = createRuntimeComponent(
        m_SceneState->m_RuntimeCompManager->getComponentDesc(compName));
}
void Entity::removeRuntimeComponent(const std::string& compName) {
    m_SceneState->m_RuntimeCompsMap[getHandle()].erase(compName);
}
