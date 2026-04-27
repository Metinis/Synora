#include "SynoraEngine/scene/Scene.h"

#include "../../include/SynoraEngine/renderer/Renderer.h"
#include "SynoraEngine/core/Input.h"
#include "SynoraEngine/core/InputContext.h"
#include "SynoraEngine/core/InputTypes.h"
#include "SynoraEngine/core/Window.h"
#include "SynoraEngine/project/AssetManager.h"
#include "SynoraEngine/scene/Components.h"
#include "glm/ext.hpp"
#include "imgui.h"
#include "renderer/backends/vulkan/Backend.h"
#include "spdlog/spdlog.h"

using namespace SYN;

Scene::Scene() {}

Entity Scene::getEntity(UUID id) {
    if (m_EntityUUIDCache.contains(id)) {
        return m_EntityUUIDCache[id];
    }
    for (auto view = getEntities<UUIDComp>(); auto entity : view) {
        if (entity.getUUID() == id) {
            m_EntityUUIDCache[id] = entity;
            return entity;
        }
    }
    return Entity{};
}

void Scene::onUpdate(float dt) {
    // clear any ref queue
    if (!m_OnUpdate.empty()) {
        for (auto &f : m_OnUpdate) {
            f();
        }
        m_OnUpdate.clear();
    }

    for (auto e : m_EntityCache) {
        auto &tc = e.getComponent<TransformComp>();
        glm::mat4 local = tc.getLocalMatrix();

        if (ParentComp *parent = e.tryGetComponent<ParentComp>()) {
            auto parentEntity = getEntity(parent->id);
            tc.worldMatrix =
                parentEntity.getComponent<TransformComp>().worldMatrix * local;
        } else {
            tc.worldMatrix = local;
        }
    }
}

void Scene::onAttach() { spdlog::debug("Scene: Attached"); }

void Scene::onDettach() { spdlog::debug("Scene: Detached"); }

void Scene::onRender() {
    for (auto &e : getEntities<MeshComp>()) {
        auto &modelComp = e.getComponent<MeshComp>();
        auto &tc = e.getComponent<TransformComp>();

        m_Renderer->drawMesh(modelComp.id, tc.worldMatrix);
    }
}

void Scene::onMeshAdded(entt::registry &reg, entt::entity e) {
    auto &comp = reg.get<MeshComp>(e);

    m_OnUpdate.push_back([&]() { m_AssetManager->addRef(comp.id); });
}

void Scene::onMeshRemoved(entt::registry &reg, entt::entity e) {
    auto &comp = reg.get<MeshComp>(e);

    m_OnUpdate.push_back([&]() { m_AssetManager->removeRef(comp.id); });
}

void Scene::onParentAdded(entt::registry &reg, entt::entity e) {
    auto comp = reg.get<ParentComp>(e);

    int depth = 1;
    auto parentEntity = getEntity(comp.id);
    while (parentEntity.hasComponent<ParentComp>()) {
        depth++;
        comp = parentEntity.getComponent<ParentComp>();
        parentEntity = getEntity(comp.id);
    }
    m_SceneState.registry.get<TransformComp>(e).depth = depth;
    std::ranges::sort(m_EntityCache, [&](Entity a, Entity b) {
        return a.getComponent<TransformComp>().depth <
               b.getComponent<TransformComp>().depth;
    });
}

void Scene::onParentRemoved(entt::registry &reg, entt::entity e) {
    Entity ent(&m_SceneState.registry, e);
    if (ent.hasComponent<TransformComp>()) {
        m_SceneState.registry.get<TransformComp>(e).depth = 0;
        std::ranges::sort(m_EntityCache, [&](Entity a, Entity b) {
            return a.getComponent<TransformComp>().depth <
                   b.getComponent<TransformComp>().depth;
        });
    }
}

bool Scene::isValidEntity(Entity entity) {
    return m_SceneState.registry.valid(entity.getHandle());
}

void Scene::onUUIDRemoved(entt::registry &reg, entt::entity e) {
    // same as removing entity, remove any entity that had this as parent
}

void Scene::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    m_Window = ctx->window.get();
    m_AssetManager = ctx->projectConfig.assetManager.get();

    // initialize callbacks
    m_SceneState.registry.on_destroy<UUIDComp>().connect<&Scene::onUUIDRemoved>(
        this);

    m_SceneState.registry.on_construct<MeshComp>().connect<&Scene::onMeshAdded>(
        this);
    m_SceneState.registry.on_destroy<MeshComp>().connect<&Scene::onMeshRemoved>(
        this);

    m_SceneState.registry.on_construct<ParentComp>()
        .connect<&Scene::onParentAdded>(this);
    m_SceneState.registry.on_destroy<ParentComp>()
        .connect<&Scene::onParentRemoved>(this);

    m_AssetManager->loadModel(this, "resources/assets/Cabin/scene.gltf");

    auto cam = createEntity("Primary Camera");
    auto &tc = cam.getComponent<TransformComp>();
    tc.position = glm::vec3(0.f, 0.f, -2.f);
    tc.rotation = glm::quat(0.f, 0.f, 0.f, 0.f);
    tc.scale = glm::vec3(1.f, 1.f, 1.f);

    auto &c = cam.addComponent<CameraComp>();
    c.fovDegrees = 90.f;
    c.aspectRatio = 16.f / 9.f;
    c.nearPlane = 0.1;
    c.farPlane = 100.f;
    c.isPrimary = true;

    auto e1 = createEntity("Parent Entity");
    auto e2 = createEntity("Parent Child Entity");
    auto e3 = createEntity("Child Entity");
    e2.addComponent<ParentComp>(e1.getUUID());
    e3.addComponent<ParentComp>(e2.getUUID());

    for (auto e : getEntities<TransformComp>()) {
        if (e.isValid()) {
            m_EntityCache.push_back(e);
        }
    }
}

Entity Scene::createEntity(const std::string &tag) {
    auto ent = Entity(&m_SceneState, m_SceneState.registry.create());
    auto id = generateUUID();
    ent.addComponent<UUIDComp>(id);
    spdlog::debug("Creating entity {}", id);
    ent.addComponent<TagComp>(TagComp{.tag = tag});
    ent.addComponent<TransformComp>();
    m_EntityCache.push_back(ent);
    m_EntityUUIDCache[id] = ent;
    return ent;
}

void Scene::removeEntity(Entity entity) {
    if (!entity.isValid())
        return;

    m_EntityUUIDCache.erase(entity.getUUID());

    std::vector<Entity> childrenToRemove;
    for (auto &ent : getEntities<ParentComp>()) {
        if (ent.isValid() &&
            ent.getComponent<ParentComp>().id == entity.getUUID()) {
            childrenToRemove.push_back(ent);
        }
    }

    for (auto &child : childrenToRemove) {
        removeEntity(child);
    }

    spdlog::debug("Removing entity {}", entity.getUUID());
    m_SceneState.registry.destroy(entity.getHandle());

    m_EntityCache.clear();
    for (auto e : getEntities<TransformComp>()) {
        if (e.isValid()) {
            m_EntityCache.push_back(e);
        }
    }
}
