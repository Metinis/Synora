#include "PuzzleEngine/Scene/Scene.h"

#include "PuzzleEngine/project/AssetManager.h"
#include "PuzzleEngine/Scene/Components.h"
#include "renderer/IBackend.h"
#include "renderer/Renderer.h"
#include "spdlog/spdlog.h"

using namespace SYN;

Scene::Scene(){

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
    for (auto &e : getEntities<MeshComp>()) {
        auto& meshComp = e.getComponent<MeshComp>();
        m_Renderer->drawMesh(meshComp.id);
    }
}

void Scene::init(Renderer* renderer, AssetManager* assetManager) {
    m_Renderer = renderer;
    //testing
    auto e = createEntity();
    MeshComp mesh{};
    auto id = assetManager->addAsset<MeshData>(MeshData{});
    e.addComponent<MeshComp>(MeshComp{.id = id});
}

Entity Scene::createEntity() {
    return Entity(&m_SceneState, m_SceneState.registry.create());
}
