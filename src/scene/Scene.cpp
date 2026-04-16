#include "PuzzleEngine/scene/Scene.h"

#include "imgui.h"
#include "PuzzleEngine/core/Input.h"
#include "PuzzleEngine/core/InputContext.h"
#include "PuzzleEngine/core/InputTypes.h"
#include "PuzzleEngine/core/Window.h"
#include "PuzzleEngine/project/AssetManager.h"
#include "PuzzleEngine/scene/Components.h"
#include "glm/ext.hpp"
#include "renderer/Renderer.h"
#include "renderer/backends/vulkan/Backend.h"
#include "spdlog/spdlog.h"


using namespace SYN;

Scene::Scene() {}


Entity Scene::getEntity(UUID id) {
    auto view = getEntities<UUIDComp>();
    for (auto entity : view) {
        if (entity.getComponent<UUIDComp>().id == id) {
            return entity;
        }
    }
    return Entity{};
}

void Scene::onUpdate(float dt) {
    //clear any ref queue
    if (!m_OnUpdate.empty()) {
        for (auto& f : m_OnUpdate) {
            f();
        }
        m_OnUpdate.clear();
    }

    for (auto e : m_EntityCache) {
        auto& tc = e.getComponent<TransformComp>();
        glm::mat4 local = tc.getLocalMatrix();

        if (auto parent = e.tryGetComponent<ParentComp>()) {
            auto parentEntity = getEntity(parent->id);
            tc.worldMatrix = parentEntity.getComponent<TransformComp>().worldMatrix * local;
        } else {
            tc.worldMatrix = local;
        }
    }

}

void Scene::onAttach() { spdlog::debug("Scene: Attached"); }

void Scene::onDettach() { spdlog::debug("Scene: Dettached"); }

void Scene::onRender() {

    for (auto &e : getEntities<MeshComp>()) {
        auto &modelComp = e.getComponent<MeshComp>();
        auto &tc = e.getComponent<TransformComp>();

        m_Renderer->drawMesh(modelComp.id, tc.worldMatrix);
    }
}

void Scene::onMeshAdded(entt::registry& reg, entt::entity e) {
    auto& comp = reg.get<MeshComp>(e);

    m_OnUpdate.push_back([&]() {
        m_AssetManager->addRef(comp.id);
    });

}

void Scene::onMeshRemoved(entt::registry& reg, entt::entity e) {
    auto& comp = reg.get<MeshComp>(e);

    m_OnUpdate.push_back([&]() {
        m_AssetManager->removeRef(comp.id);
    });
}

void Scene::onParentAdded(entt::registry &reg, entt::entity e) {
    auto& comp = reg.get<ParentComp>(e);

    m_OnUpdate.push_back([&]() {
        int depth = 1;
        auto parentEntity = getEntity(comp.id);
        while (parentEntity.hasComponent<ParentComp>()) {
            depth++;
            comp = parentEntity.getComponent<ParentComp>();
        }
        m_SceneState.registry.get<TransformComp>(e).depth = depth;
        std::ranges::sort(m_EntityCache,
                      [&](Entity a, Entity b) {
                          return a.getComponent<TransformComp>().depth < b.getComponent<TransformComp>().depth;
                      });
    });
}

void Scene::onParentRemoved(entt::registry &reg, entt::entity e) {
    m_OnUpdate.push_back([&]() {
        m_SceneState.registry.get<TransformComp>(e).depth = 0;
        std::ranges::sort(m_EntityCache,
                      [&](Entity a, Entity b) {
                          return a.getComponent<TransformComp>().depth < b.getComponent<TransformComp>().depth;
                      });
    });
}

bool Scene::isValidEntity(Entity entity) {
    return m_SceneState.registry.valid(entity.getHandle());
}

void Scene::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    m_Window = ctx->window.get();
    m_AssetManager = ctx->projectConfig.assetManager.get();

    //initialize callbacks
    m_SceneState.registry.on_construct<MeshComp>().connect<&Scene::onMeshAdded>(this);
    m_SceneState.registry.on_destroy<MeshComp>().connect<&Scene::onMeshRemoved>(this);

    m_SceneState.registry.on_construct<ParentComp>().connect<&Scene::onParentRemoved>(this);
    m_SceneState.registry.on_destroy<ParentComp>().connect<&Scene::onParentAdded>(this);

    m_AssetManager->loadModel(this,
        "resources/assets/Cabin/scene.gltf");

    auto cam = createEntity("Primary Camera");
    auto& tc = cam.getComponent<TransformComp>();
    tc.position = glm::vec3(0.f, 0.f, -2.f);
    tc.rotation = glm::quat(0.f, 0.f, 0.f, 0.f);
    tc.scale = glm::vec3(1.f, 1.f, 1.f);

    auto& c = cam.addComponent<CameraComp>();
    c.fovDegrees = 90.f;
    c.aspectRatio = 16.f / 9.f;
    c.nearPlane = 0.0001;
    c.farPlane = 100.f;
    c.isPrimary = true;

    auto parent = createEntity("Parent Entity");
    auto child = createEntity("Child Entity");
    child.addComponent<ParentComp>(parent.getComponent<UUIDComp>().id);
}

Entity Scene::createEntity(const std::string &tag) {
    auto ent = Entity(&m_SceneState, m_SceneState.registry.create());
    ent.addComponent<UUIDComp>(generateUUID());
    ent.addComponent<TagComp>(TagComp{.tag = tag});
    ent.addComponent<TransformComp>();
    m_EntityCache.push_back(ent);
    return ent;
}

void Scene::removeEntity(Entity entity) {
    m_SceneState.registry.destroy(entity.getHandle());
    auto& vec = m_EntityCache;
    std::erase_if(vec,
                  [&](const Entity& e) {
                      return entity.getHandle() == entity.getHandle();
                  });
}
