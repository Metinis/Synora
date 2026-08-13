#include "EditorPanel.h"
#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/scene/ScriptManager.h"
#include "imgui.h"

namespace SYE {
  void EditorPanel::onMenuBarPanelRender() {
    ImGuiViewport* viewport = ImGui::GetMainViewport();

    if (ImGui::BeginMainMenuBar()){

        if (ImGui::BeginMenu("File")) {
          if (ImGui::MenuItem("New")) {
          }
          if (ImGui::MenuItem("Open")) {
          }
          if (ImGui::MenuItem("Save")) {
          }
          ImGui::Separator();
          if (ImGui::MenuItem("Exit")) {
          }
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
          if (ImGui::MenuItem("Undo")) {}
          if (ImGui::MenuItem("Redo")) {}
          ImGui::Separator();
          if (ImGui::MenuItem("Cut")) {}
          if (ImGui::MenuItem("Copy")) {}
          if (ImGui::MenuItem("Paste")) {}
          ImGui::EndMenu();
        }

        bool& isGameRunning = SYN::Application::get().getCtx()->isGameRunning;

        if (ImGui::BeginMenu("Run")) {
          if(!isGameRunning) {
            if(ImGui::MenuItem("Start")) {
              isGameRunning = true;
              SYN::Application::get().getCtx()->scriptManager.get()->loadAllSystems();
            }          
          }
          if(isGameRunning) {
            if(ImGui::MenuItem("Stop")) {
              isGameRunning = false;
            }          
          }
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
          if (ImGui::MenuItem("Settings")) {}
          if (ImGui::MenuItem("Console")) {}
          ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
          if (ImGui::MenuItem("About")) {}
          ImGui::EndMenu();
        }

      ImGui::EndMainMenuBar();

    }
  }
}
