#include "EditorPanel.h"

using namespace SYE;

void EditorPanel::init(SYN::EngineContext *ctx) {
    // m_Scene = ctx->scene.get();
    // m_AssetManager = ctx->projectConfig.assetManager.get();
}
void EditorPanel::onUIRender() {
    onScenePanelRender();
    onInspectorPanelRender();
}
