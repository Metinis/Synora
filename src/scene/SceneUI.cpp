#include <PuzzleEngine/scene/Scene.h>
#include "imgui.h"
#include "tinyfiledialogs.h"
#include "glm/gtc/type_ptr.hpp"
#include "PuzzleEngine/project/AssetManager.h"

using namespace SYN;

void Scene::onUIRender() {
    ImGui::Begin("Scene UI");
    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::SetWindowSize(ImVec2(200, 600));

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
    auto inspectModel = [this](Entity e, ModelComp* mc) {
        ImGui::Text("Model");
        ImGui::SameLine();
        if (!mc) {
            if (ImGui::Button("Add")) {
                std::array filters = { "*.obj", "*.fbx", "*.glb", "*.gltf", "*.mtl" };
                const char* path = tinyfd_openFileDialog("Choose a model", "",
                    filters.size(), filters.data(), "3D Model Files", 1);
                if (path) {
                    UUID uuid{m_AssetManager->loadModel(path)};
                    spdlog::info("uuid = {}", uuid);

                    e.addComponent<ModelComp>(ModelComp{.id = uuid});
                }
            }
        } else {
            if (ImGui::Button("Remove")) {
                e.removeComponent<ModelComp>();
            }
        }
    };

    for (auto &e : getEntities<TransformComp>()) {
        ImGui::PushID(e.getComponent<UUIDComp>().id);
        //Have dropdown of all entities
        if (ImGui::TreeNode(e.getComponent<TagComp>().tag.c_str())) {
            if (ImGui::CollapsingHeader("Transform")) {
                auto* tc = e.tryGetComponent<TransformComp>();
                inspectTransform(tc);
            }
            if (ImGui::CollapsingHeader("Model")) {
                auto* mc = e.tryGetComponent<ModelComp>();
                inspectModel(e, mc);
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

    bool openCreateEntity = false;
    if (ImGui::BeginPopupContextWindow("ScenePanelContext", ImGuiPopupFlags_MouseButtonRight |
        ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Entity")) {
            openCreateEntity = true;
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