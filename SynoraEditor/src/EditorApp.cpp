#include "EditorApp.h"
#include "EditorPanel.h"
using namespace SYE;

EditorApp::EditorApp() {
    m_ScenePanel = std::make_unique<EditorPanel>();
}
void EditorApp::init() {
    Application::init();
    m_ScenePanel->init(&m_EngineContext);
    m_Layers.push_front(m_ScenePanel.get());
}
EditorApp::~EditorApp() {}