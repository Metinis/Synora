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
#include "spdlog/spdlog.h"



using namespace SYN;

Scene::Scene() {}



void Scene::onUpdate(float dt) {

}

void Scene::onAttach() { spdlog::debug("Scene: Attached"); }

void Scene::onDettach() { spdlog::debug("Scene: Dettached"); }

void Scene::onRender() {

    for (auto &e : getEntities<ModelComp>()) {
        auto &modelComp = e.getComponent<ModelComp>();
        auto &tc = e.getComponent<TransformComp>();

        m_Renderer->drawModel(modelComp.id, tc);
    }
}

void Scene::onUIRender() {
    ImGui::Begin("Scene UI");
    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::SetWindowSize(ImVec2(200, 600));

    auto inspectTransform = [](TransformComp& tc) {
        ImGui::DragFloat3("Position", glm::value_ptr(tc.position), 0.1f);
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(tc.eulerAngles), 0.1f)) {
            tc.rotation = glm::quat(glm::radians(tc.eulerAngles));
        }
        ImGui::DragFloat3("Scale", glm::value_ptr(tc.scale), 0.1f);
    };

    ImGui::BeginGroup();
    for (auto &e : getEntities<TransformComp>()) {
        ImGui::PushID(e.getComponent<UUIDComp>().id);
        //Have dropdown of all entities
        if (ImGui::TreeNode(e.getComponent<TagComp>().tag.c_str())) {
            if (ImGui::CollapsingHeader("Transform")) {
                auto& tc = e.getComponent<TransformComp>();
                inspectTransform(tc);
            }

            ImGui::TreePop();
        }
        if (ImGui::BeginPopupContextItem("EntityPanelContext", ImGuiPopupFlags_MouseButtonRight)) {
            if (ImGui::MenuItem("Delete")) {
                removeEntity(e);
            }

            ImGui::EndPopup();
        }
        ImGui::PopID();

    }
    ImGui::EndGroup();
    if (ImGui::BeginPopupContextItem("ScenePanelContext", ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("Create Entity")) {
            ImGui::BeginPopup("Entity Popup");
            static char entityName[256] = "";
            ImGui::InputText("##EntityName", entityName, IM_ARRAYSIZE(entityName));
            ImGui::Separator();

            if (ImGui::Button("Create", ImVec2(120, 0))) {
                createEntity(entityName);
            }
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::End();
}

bool Scene::isValidEntity(Entity entity) {
    return m_SceneState.registry.valid(entity.getHandle());
}

void Scene::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    m_Window = ctx->window.get();

    auto e = createEntity();
    UUID uuid{ctx->projectConfig.assetManager->loadModel(
        "resources/assets/Cabin/scene.gltf")};
    spdlog::info("uuid = {}", uuid);

    e.addComponent<ModelComp>(ModelComp{.id = uuid});

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
}

Entity Scene::createEntity(const std::string &tag) {
    auto ent = Entity(&m_SceneState, m_SceneState.registry.create());
    ent.addComponent<UUIDComp>(generateUUID());
    ent.addComponent<TagComp>(TagComp{.tag = tag});
    ent.addComponent<TransformComp>();
    return ent;
}

void Scene::removeEntity(Entity entity) {
    m_SceneState.registry.destroy(entity.getHandle());
}
