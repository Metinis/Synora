#pragma once
#include <SynoraEngine/scene/ISystem.h>

namespace SYN {
class RuntimeCompManager;
}
class TestSystem : public SYN::ISystem {
public:
    TestSystem() = default;
    void init(SYN::EngineContext* ctx) override;
    void onLoad() override;
    void onUpdate(float dt) override;
    void onUnload() override;
    ~TestSystem() override = default;
};

extern "C" {
    SYN::ISystem* createSystem();
    void destroySystem(SYN::ISystem* system);
    void registerComponents(SYN::RuntimeCompManager* manager);
}

//Components example
struct Test {
    float i;
};
