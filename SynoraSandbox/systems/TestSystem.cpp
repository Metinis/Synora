#include "TestSystem.h"
#include "SynoraEngine/core/Application.h"
#include <SynoraEngine/scene/Scene.h>

void TestSystem::init(SYN::EngineContext *ctx) {
    m_Ctx = ctx;
}
void TestSystem::onLoad() {
    std::cout << "Test System Loaded" << std::endl;

}
void TestSystem::onUpdate(float dt) {
    for (auto e : m_Ctx->scene->getEntitiesRuntime("Test")) {
        e.getComponent<TransformComp>().position.x += 1.0f * dt ;
    }
}
void TestSystem::onUnload() {

}

extern "C" SYN::ISystem* createSystem() {
    return new TestSystem();
}

extern "C" void destroySystem(SYN::ISystem* system) {
    delete system;
}
extern "C" void registerComponents(SYN::RuntimeCompManager* manager) {
    REGISTER_COMPONENT(manager, Test, FIELD(Test, i));
}