#include "EditorPanel.h"
#include "glm/gtc/type_ptr.hpp"
#include "imgui_internal.h"
#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/project/AssetManager.h>
#include <SynoraEngine/scene/Scene.h>
#include <imgui.h>
#include <tinyfiledialogs.h>

using namespace SYE;

static constexpr const char* ENTITY_DRAG_PAYLOAD = "SYN_ENTITY";

void EditorPanel::onScenePanelRender() {
    ImGui::Begin("Scene UI");
    ImGui::SetWindowPos(ImVec2(0, 20), ImGuiCond_Once);
    ImGui::SetWindowSize(ImVec2(200, 400), ImGuiCond_Once);
    ImGui::BeginChild("SceneHierarchyRegion", ImVec2(0, 0), false);

    std::vector<SYN::Entity> deleteQueue;
    for (auto &e : m_Scene->getEntities<TransformComp>()) {
        if (!e.hasComponent<ParentComp>())
            drawEntityNode(e, deleteQueue);
    }
    for (const auto e : deleteQueue) {
        m_Scene->removeEntity(e);
    }
    if (ImGui::BeginDragDropTargetCustom(  ImGui::GetCurrentWindow()->Rect(),
            ImGui::GetID("SceneRootDrop"))) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ENTITY_DRAG_PAYLOAD)) {

            SYN::UUID droppedID = *(const SYN::UUID*)payload->Data;
            m_PendingReparent = { droppedID, 0 };

        }

        ImGui::EndDragDropTarget();

    }
    if (m_PendingReparent) {
        auto& [childID, parentID] = *m_PendingReparent;

        auto child  = m_Scene->getEntity(childID);
        auto parent = m_Scene->getEntity(parentID);

        if (child.isValid() && parent.isValid() &&
            !m_Scene->isDescendantOf(parent, child))
        {
            if (!child.hasComponent<ParentComp>()) {
                child.addParent(ParentComp{.id = parentID});
            } else {
                child.getComponent<ParentComp>().id = parentID;
            }
        } else if (child.isValid() && parentID == 0) {
            if (child.hasComponent<ParentComp>()) {
                child.removeComponent<ParentComp>();
            }
        }

        m_PendingReparent.reset();
    }

    bool openCreateEntity = false;
    if (ImGui::BeginPopupContextWindow("ScenePanelContext",
                                       ImGuiPopupFlags_MouseButtonRight |
                                           ImGuiPopupFlags_NoOpenOverItems)) {
        if (ImGui::MenuItem("Create Entity")) {
            openCreateEntity = true;
        }
        if (ImGui::MenuItem("Import Model")) {
            std::array filters = {"*.obj", "*.fbx", "*.glb", "*.gltf", "*.mtl"};
            const char *path =
                tinyfd_openFileDialog("Choose a model", "", filters.size(),
                                      filters.data(), "3D Model Files", 1);
            if (path) {
                m_AssetManager->loadModel(m_Scene, path);
            }
        }

        ImGui::EndPopup();
    }
    if (openCreateEntity) {
        ImGui::OpenPopup("EntityPopup");
    }

    if (ImGui::BeginPopupModal("EntityPopup", nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
        static char entityName[256] = "Unnamed Entity";
        ImGui::Text("Enter Entity Name: ");
        ImGui::InputText("##EntityName", entityName, IM_ARRAYSIZE(entityName));
        ImGui::Separator();

        if (ImGui::Button("Create", ImVec2(120, 0))) {
            m_Scene->createEntity(entityName);
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
    ImGui::EndChild();
    ImGui::End();
}

void EditorPanel::drawEntityNode(SYN::Entity entity,
                                 std::vector<SYN::Entity> &deletionQueue) {
    // Have dropdown of all entities
    ImGuiTreeNodeFlags flags =
        (m_SelectedEntity.getHandle() == entity.getHandle()
             ? ImGuiTreeNodeFlags_Selected
             : 0) |
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;

    bool opened =
        ImGui::TreeNodeEx(entity.getComponent<TagComp>().tag.c_str(), flags);

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        SYN::Entity dragged = entity;
        ImGui::SetDragDropPayload(ENTITY_DRAG_PAYLOAD, &dragged.getComponent<UUIDComp>().id,
                sizeof(dragged.getComponent<UUIDComp>().id));
        //ImGui::Text("Move %s", name.tag.c_str());
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ENTITY_DRAG_PAYLOAD)) {

            SYN::UUID droppedID = *(const SYN::UUID*)payload->Data;
            SYN::UUID targetID  = entity.getComponent<UUIDComp>().id;

            if (droppedID != targetID) {
                m_PendingReparent = { droppedID, targetID };
            }
        }

        ImGui::EndDragDropTarget();

    }

    if (ImGui::IsItemClicked()) {
        m_SelectedEntity = entity;
    }

    ImGui::PushID((int)entity.getHandle());
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Delete")) {
            deletionQueue.push_back(entity);
        }
        ImGui::EndPopup();
    }
    // draw children
    if (opened) {
        for (auto e : m_Scene->getEntities<ParentComp>()) {
            if (e.getComponent<ParentComp>().id == entity.getUUID()) {
                drawEntityNode(e, deletionQueue);
            }
        }
        ImGui::TreePop();
    }

    ImGui::PopID();

}

SYE::EditorPanel::~EditorPanel() {}
