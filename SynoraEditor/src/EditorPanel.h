#pragma once
#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/project/AssetManager.h"
#include "SynoraEngine/scene/Entity.h"

#include <SynoraEngine/core/Layer.h>

namespace SYN {
struct EngineContext;
}
namespace SYE {
struct PendingReparent {
    SYN::UUID child;
    SYN::UUID newParent; //null UUID = unparent
};
class EditorPanel : public SYN::ILayer {
public:
    EditorPanel() = default;
    void init(SYN::EngineContext* ctx);
    void onUIRender() override;
    void onAttach() override  {};
    void onDetach() override  {};
    void onRender() override  {};
    void onUpdate(float dt) override {};
    void drawEntityNode(SYN::Entity entity, std::vector<SYN::Entity>& deletionQueue);
    void onScenePanelRender();
    void onInspectorPanelRender();
    void onMenuBarPanelRender();

    ~EditorPanel() override;
private:
    SYN::Entity m_SelectedEntity{};
    SYN::EngineContext* m_Ctx{};
    std::optional<PendingReparent> m_PendingReparent;
};
}
