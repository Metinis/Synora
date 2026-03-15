#include "PuzzleEngine/Scene/Scene.h"

#include "PuzzleEngine/Scene/Components.h"

using namespace SYN;

Scene::Scene() {

}

void Scene::init() {
    //testing
    auto e = createEntity();
    MeshComp mesh{};
    e.addComponent<MeshComp>(mesh);
}

Entity Scene::createEntity() {
    return Entity(&m_SceneState, m_SceneState.registry.create());
}
