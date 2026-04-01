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
    for (auto &e : getEntities<ModelComp>()) {
        auto &modelComp = e.getComponent<ModelComp>();

        m_Renderer->drawModel(modelComp.id, {0.f, -0.5f, 2.f});
    }
}

void Scene::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    // testing
    auto e = createEntity();
    UUID uuid{ctx->projectConfig.assetManager->loadModel(
        "resources/assets/Test/test.obj")};
    spdlog::info("uuid = {}", uuid);

    e.addComponent<ModelComp>(ModelComp{.id = uuid});
}

Entity Scene::createEntity() {
    return Entity(&m_SceneState, m_SceneState.registry.create());
}
