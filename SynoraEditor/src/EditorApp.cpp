#include "EditorApp.h"
#include "ScenePanel.h"
using namespace SYE;

EditorApp::EditorApp() {
    m_ScenePanel = std::make_unique<ScenePanel>();
}
void EditorApp::init() {
    Application::init();
    m_ScenePanel->init(&m_EngineContext);
    m_Layers.push_front(m_ScenePanel.get());
}
EditorApp::~EditorApp() {}