#pragma once
#include "SynoraEngine/core/Application.h"

namespace SYE {
class EditorPanel;

class EditorApp : public SYN::Application {
    public:
    EditorApp();
    void init(const std::filesystem::path& path = "") override;
    ~EditorApp() override;
    private:
    std::unique_ptr<EditorPanel> m_ScenePanel{};
    };
}
