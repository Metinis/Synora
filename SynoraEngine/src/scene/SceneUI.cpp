#include <SynoraEngine/scene/Scene.h>
#include "imgui.h"
#include "tinyfiledialogs.h"
#include "glm/gtc/type_ptr.hpp"
#include "SynoraEngine/project/AssetManager.h"

using namespace SYN;



void Scene::onUIRender() {
    ImGui::Begin("Scene UI");
    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::SetWindowSize(ImVec2(200, 600));

    std::vector<Entity> deleteQueue;
    for (auto &e : getEntities<TransformComp>()) {
        if (!e.hasComponent<ParentComp>())
            drawEntityNode(e, deleteQueue);
    }
    for (const auto e : deleteQueue) {
        removeEntity(e);
    }

    bool openCreateEntity = false;
    if (ImGui::BeginPopupContextWindow("ScenePanelContext", ImGuiPopupFlags_MouseButtonRight |
        ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Entity")) {
            openCreateEntity = true;
        }
        if (ImGui::MenuItem("Import Model")) {
            std::array filters = { "*.obj", "*.fbx", "*.glb", "*.gltf", "*.mtl" };
            const char* path = tinyfd_openFileDialog("Choose a model", "",
                filters.size(), filters.data(), "3D Model Files", 1);
            if (path) {
                m_AssetManager->loadModel(this, path);
            }
        }

        ImGui::EndPopup();
    }
    if (openCreateEntity) {
        ImGui::OpenPopup("EntityPopup");
    }

    if (ImGui::BeginPopupModal("EntityPopup", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        static char entityName[256] = "Unnamed Entity";
        ImGui::Text("Enter Entity Name: ");
        ImGui::InputText("##EntityName", entityName, IM_ARRAYSIZE(entityName));
        ImGui::Separator();

        if (ImGui::Button("Create", ImVec2(120, 0))) {
            createEntity(entityName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}
auto inspectTransform = [](TransformComp* tc) {
    if (!tc) {
        return;
    }
    ImGui::DragFloat3("Position", glm::value_ptr(tc->position), 0.1f);
    if (ImGui::DragFloat3("Rotation", glm::value_ptr(tc->eulerAngles), 0.1f)) {
        tc->rotation = glm::quat(glm::radians(tc->eulerAngles));
    }
    ImGui::DragFloat3("Scale", glm::value_ptr(tc->scale), 0.1f);
};
auto inspectMesh = [](Entity e, MeshComp* mc) {
    ImGui::Text("Mesh");
    ImGui::SameLine();
    if (!mc) {
        if (ImGui::Button("Add")) {
            //TODO
        }
    } else {
        if (ImGui::Button("Remove")) {
            e.removeComponent<MeshComp>();
        }
    }
};
void Scene::drawEntityNode(Entity entity, std::vector<Entity>& deletionQueue) {
    //Have dropdown of all entities
    if (ImGui::TreeNode(entity.getComponent<TagComp>().tag.c_str())) {
        ImGui::PushID((int)entity.getHandle());
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete")) {
                deletionQueue.push_back(entity);
            }
            ImGui::EndPopup();
        }

        if (ImGui::CollapsingHeader("Transform")) {
            auto* tc = entity.tryGetComponent<TransformComp>();
            inspectTransform(tc);
        }
        if (ImGui::CollapsingHeader("Model")) {
            auto* mc = entity.tryGetComponent<MeshComp>();
            inspectMesh(entity, mc);
        }

        //draw children
        for (auto e : getEntities<ParentComp>()) {
            if (e.getComponent<ParentComp>().id == entity.getUUID()) {
                drawEntityNode(e, deletionQueue);
            }
        }

        ImGui::PopID();
        ImGui::TreePop();
    }
}
