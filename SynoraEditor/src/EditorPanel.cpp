#include "EditorPanel.h"

using namespace SYE;

void EditorPanel::init(SYN::EngineContext *ctx) {
  m_Ctx = ctx;
}
void EditorPanel::onUIRender() {
    onScenePanelRender();
    onInspectorPanelRender();
    onMenuBarPanelRender();
}
