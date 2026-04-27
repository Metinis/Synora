#pragma once
#include "SynoraEngine/project/AssetManager.h"
#include "SynoraEngine/scene/Entity.h"

#include <SynoraEngine/core/Layer.h>

namespace SYN {
struct EngineContext;
}
namespace SYE {
class ScenePanel : public SYN::ILayer {
public:
    ScenePanel() = default;
    void init(SYN::EngineContext* ctx);
    void onUIRender() override;
    void onAttach() override  {};
    void onDettach() override  {};
    void onRender() override  {};
    void onUpdate(float dt) override {};
    void drawEntityNode(SYN::Entity entity, std::vector<SYN::Entity>& deletionQueue);

    ~ScenePanel() override;
private:
    SYN::Scene* m_Scene;
    SYN::AssetManager* m_AssetManager;
};
}
