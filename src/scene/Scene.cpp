#include "PuzzleEngine/Scene/Scene.h"

#include "PuzzleEngine/Scene/Components.h"
#include "spdlog/spdlog.h"

using namespace SYN;

Scene::Scene() {

}

void Scene::onUpdate(float dt) {
    //spdlog::debug("Scene: Update");
}

void Scene::onAttach() {
    spdlog::debug("Scene: Attached");
}

void Scene::onDettach() {
    spdlog::debug("Scene: Dettached");
}

void Scene::onRender() {
    //spdlog::debug("Scene: Render");
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
