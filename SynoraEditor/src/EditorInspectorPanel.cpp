#include "EditorPanel.h"
#include "glm/gtc/type_ptr.hpp"
#include "imgui.h"
#include <SynoraEngine/project/Assets.h>

using namespace SYE;

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


void EditorPanel::onInspectorPanelRender() {
    auto inspectMesh = [&](SYN::Entity e, MeshComp* mc) {
        if (!mc) {
            ImGui::Text("Mesh");
            ImGui::SameLine();
            if (ImGui::Button("Add")) {
                //TODO
                e.addComponent<MeshComp>();
            }
        } else {
            if (ImGui::BeginCombo("Mesh", "TestMesh")) {
                for (auto& meshID : m_AssetManager->getAssets<SYN::MeshData>()) {
                    if (ImGui::Selectable(std::to_string(meshID).c_str())) {
                        e.removeComponent<MeshComp>();
                        e.addComponent<MeshComp>(MeshComp{.id = meshID});
                    }
                }
                ImGui::EndCombo();
            }
            if (ImGui::Button("Remove")) {
                e.removeComponent<MeshComp>();
            }
        }
    };

    auto inspectMaterial = [&](SYN::Entity e, MeshComp* mc) {
        ImGui::Text("Material");
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
    ImGui::Begin("Inspector UI");
    ImGui::SetWindowPos(ImVec2(0, 400), ImGuiCond_Once);
    ImGui::SetWindowSize(ImVec2(200, 300), ImGuiCond_Once);

    if (m_SelectedEntity.isValid()) {
        if (ImGui::CollapsingHeader("Transform")) {
            auto* tc = m_SelectedEntity.tryGetComponent<TransformComp>();
            inspectTransform(tc);
        }
        if (ImGui::CollapsingHeader("Model")) {
            auto* mc = m_SelectedEntity.tryGetComponent<MeshComp>();
            inspectMesh(m_SelectedEntity, mc);
        }
        if (auto *mc = m_SelectedEntity.tryGetComponent<MeshComp>()) {
            if (ImGui::CollapsingHeader("Material")) {
                inspectMaterial(m_SelectedEntity, mc);
            }
        }
    }
    ImGui::End();
}