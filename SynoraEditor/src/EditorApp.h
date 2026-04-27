#pragma once
#include "SynoraEngine/core/Application.h"

namespace SYE {
class ScenePanel;

class EditorApp : public SYN::Application {
    public:
    EditorApp();
    void init() override;
    ~EditorApp() override;
    private:
    std::unique_ptr<ScenePanel> m_ScenePanel{};
    };
}
