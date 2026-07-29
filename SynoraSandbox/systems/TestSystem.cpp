#include "TestSystem.h"

void TestSystem::init(SYN::EngineContext *ctx) {
    m_Ctx = ctx;
}
void TestSystem::onLoad() {
    std::cout << "Test System Loaded" << std::endl;
}
void TestSystem::onUpdate() {

}
void TestSystem::onUnload() {

}

extern "C" SYN::ISystem* createSystem(SYN::EngineContext* ctx) {
    return new TestSystem();
}