#pragma once
#include <SynoraEngine/scene/ISystem.h>

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
}

