#include <PuzzleEngine/scene/Scene.h>
#include "imgui.h"
#include "glm/gtc/type_ptr.hpp"

using namespace SYN;
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