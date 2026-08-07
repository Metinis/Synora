#include "TestSystem.h"
#include "SynoraEngine/core/Application.h"
#include "SynoraEngine/project/AssetManager.h"
#include <SynoraEngine/scene/Scene.h>

void TestSystem::init(SYN::EngineContext *ctx) {
    m_Ctx = ctx;
}
void TestSystem::onLoad() {
    std::cout << "Test System Loaded" << std::endl;
    auto en = m_Ctx->scene->createEntity("Block");

    // Fill vertex/index data

    SYN::MeshData mesh{};

    mesh.vertices = {
        // Front (+Z)
        {{-0.5f,-0.5f, 0.5f},0.0f,{ 0, 0, 1},0.0f,{ 1,0,0,1}},
        {{ 0.5f,-0.5f, 0.5f},1.0f,{ 0, 0, 1},0.0f,{ 1,0,0,1}},
        {{ 0.5f, 0.5f, 0.5f},1.0f,{ 0, 0, 1},1.0f,{ 1,0,0,1}},
        {{-0.5f, 0.5f, 0.5f},0.0f,{ 0, 0, 1},1.0f,{ 1,0,0,1}},

        // Back (-Z)
        {{ 0.5f,-0.5f,-0.5f},0.0f,{ 0, 0,-1},0.0f,{-1,0,0,1}},
        {{-0.5f,-0.5f,-0.5f},1.0f,{ 0, 0,-1},0.0f,{-1,0,0,1}},
        {{-0.5f, 0.5f,-0.5f},1.0f,{ 0, 0,-1},1.0f,{-1,0,0,1}},
        {{ 0.5f, 0.5f,-0.5f},0.0f,{ 0, 0,-1},1.0f,{-1,0,0,1}},

        // Left (-X)
        {{-0.5f,-0.5f,-0.5f},0.0f,{-1, 0, 0},0.0f,{0,0,1,1}},
        {{-0.5f,-0.5f, 0.5f},1.0f,{-1, 0, 0},0.0f,{0,0,1,1}},
        {{-0.5f, 0.5f, 0.5f},1.0f,{-1, 0, 0},1.0f,{0,0,1,1}},
        {{-0.5f, 0.5f,-0.5f},0.0f,{-1, 0, 0},1.0f,{0,0,1,1}},

        // Right (+X)
        {{ 0.5f,-0.5f, 0.5f},0.0f,{ 1, 0, 0},0.0f,{0,0,-1,1}},
        {{ 0.5f,-0.5f,-0.5f},1.0f,{ 1, 0, 0},0.0f,{0,0,-1,1}},
        {{ 0.5f, 0.5f,-0.5f},1.0f,{ 1, 0, 0},1.0f,{0,0,-1,1}},
        {{ 0.5f, 0.5f, 0.5f},0.0f,{ 1, 0, 0},1.0f,{0,0,-1,1}},

        // Top (+Y)
        {{-0.5f, 0.5f, 0.5f},0.0f,{ 0, 1, 0},0.0f,{1,0,0,1}},
        {{ 0.5f, 0.5f, 0.5f},1.0f,{ 0, 1, 0},0.0f,{1,0,0,1}},
        {{ 0.5f, 0.5f,-0.5f},1.0f,{ 0, 1, 0},1.0f,{1,0,0,1}},
        {{-0.5f, 0.5f,-0.5f},0.0f,{ 0, 1, 0},1.0f,{1,0,0,1}},

        // Bottom (-Y)
        {{-0.5f,-0.5f,-0.5f},0.0f,{ 0,-1, 0},0.0f,{1,0,0,1}},
        {{ 0.5f,-0.5f,-0.5f},1.0f,{ 0,-1, 0},0.0f,{1,0,0,1}},
        {{ 0.5f,-0.5f, 0.5f},1.0f,{ 0,-1, 0},1.0f,{1,0,0,1}},
        {{-0.5f,-0.5f, 0.5f},0.0f,{ 0,-1, 0},1.0f,{1,0,0,1}},
    };

    mesh.indices = {
        // Front
        0,1,2, 2,3,0,

        // Back
        4,5,6, 6,7,4,

        // Left
        8,9,10, 10,11,8,

        // Right
        12,13,14, 14,15,12,

        // Top
        16,17,18, 18,19,16,

        // Bottom
        20,21,22, 22,23,20
    };

    mesh.albedo = nullptr;
    mesh.metallicRoughness = nullptr;
    mesh.normalMap = nullptr;
    auto meshHandle =
        m_Ctx->projectConfig.assetManager->addAsset<SYN::MeshData>(mesh);

    en.addComponent<MeshComp>(meshHandle);
    en.addRuntimeComponent("Test");

}
void TestSystem::onUpdate(float dt) {
    for (auto &e : m_Ctx->scene->getEntitiesRuntime("Test")) {
        auto& tc = e.getComponent<TransformComp>();
        tc.rotation = glm::rotate(tc.rotation, -glm::radians(dt * 50), glm::vec3(0.0f, 1.0f, 0.0f));
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