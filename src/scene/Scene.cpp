#include "PuzzleEngine/scene/Scene.h"

#include "PuzzleEngine/project/AssetManager.h"
#include "PuzzleEngine/scene/Components.h"
#include "renderer/Renderer.h"
#include "spdlog/spdlog.h"

using namespace SYN;

Scene::Scene() {}

void Scene::onUpdate(float dt) {
    // spdlog::debug("Scene: Update");
}

void Scene::onAttach() { spdlog::debug("Scene: Attached"); }

void Scene::onDettach() { spdlog::debug("Scene: Dettached"); }

void Scene::onRender() {
    // spdlog::debug("Scene: Render");
    for (auto &e : getEntities<MeshComp>()) {
        auto &meshComp = e.getComponent<MeshComp>();
        m_Renderer->drawMesh(meshComp.id);
    }
}

void Scene::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    // testing
    auto e = createEntity();
    MeshComp mesh{};
    auto id = ctx->projectConfig.assetManager->addAsset<MeshData>(MeshData{});
    e.addComponent<MeshComp>(MeshComp{.id = id});
}

Entity Scene::createEntity() {
    return Entity(&m_SceneState, m_SceneState.registry.create());
}
