#include "PuzzleEngine/scene/Scene.h"

#include "imgui.h"
#include "PuzzleEngine/core/Input.h"
#include "PuzzleEngine/core/InputContext.h"
#include "PuzzleEngine/core/InputTypes.h"
#include "PuzzleEngine/core/Window.h"
#include "PuzzleEngine/project/AssetManager.h"
#include "PuzzleEngine/scene/Components.h"
#include "glm/ext.hpp"
#include "renderer/Renderer.h"
#include "spdlog/spdlog.h"

namespace {
enum Action : SYN::ActionID {
    MoveForward,
    MoveBackward,
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    Run,
    LookDelta,
    ToggleCursor
};
}

using namespace SYN;

Scene::Scene() {}

Entity Scene::getCameraEntity() {
    for (auto e : getEntities<CameraComp>()) {
        if (e.getComponent<CameraComp>().isPrimary) {
            return e;
        }
    }
    return Entity();
}

Camera Scene::getCamera() {
    auto camEntity = getCameraEntity();

    if (!m_SceneState.registry.valid(camEntity.getHandle())) {
        spdlog::warn("Scene: No camera entity");
        return Camera();
    }

    auto camTC = camEntity.getComponent<TransformComp>();
    auto camComp = camEntity.getComponent<CameraComp>();
    Camera cam = {
        .transform = camTC,
        .fovDegrees = camComp.fovDegrees,
        .aspectRatio = camComp.aspectRatio,
        .nearPlane = camComp.nearPlane,
        .farPlane = camComp.farPlane,
    };
    return cam;
}

void Scene::onUpdate(float dt) {
    if (m_LastCursorHiddenState != m_CursorHidden) {
        m_DyRot = 0;
        m_DxRot = 0;
    }
    m_LastCursorHiddenState = m_CursorHidden;

    if (!m_CursorHidden) {
        return;
    }

    float speed{m_WalkSpeed};
    if (m_IsRunning) {
        speed *= m_RunMultiplier;
    }

    auto camera = getCameraEntity();
    auto& camTC = camera.getComponent<TransformComp>();

    glm::vec3 lookDir{camTC.rotation * glm::vec3(0.f, 0.f, 1.f)};
    lookDir.y = 0.f;
    lookDir = glm::normalize(lookDir);

    glm::vec3 upDir(0.f, 1.f, 0.f);
    glm::vec3 sideDir{glm::cross(lookDir, upDir)};

    glm::vec3 dPos{(lookDir * m_Dz) + (sideDir * m_Dx) + (upDir * -m_Dy)};
    dPos *= dt * speed;

    camTC.position += dPos;

    float sensitivity{0.1f};
    glm::quat yaw{
        glm::angleAxis(glm::radians(static_cast<float>(-m_DxRot * sensitivity)),
                       glm::vec3(0.f, 1.f, 0.f))};
    glm::quat pitch{
        glm::angleAxis(glm::radians(static_cast<float>(m_DyRot * sensitivity)),
                       glm::vec3(1.f, 0.f, 0.f))};

    camTC.rotation =
        glm::normalize(yaw * camTC.rotation * pitch);




    m_DyRot = 0;
    m_DxRot = 0;
}

void Scene::onAttach() { spdlog::debug("Scene: Attached"); }

void Scene::onDettach() { spdlog::debug("Scene: Dettached"); }

void Scene::onRender() {
    m_Renderer->setCamera(getCamera());
    for (auto &e : getEntities<ModelComp>()) {
        auto &modelComp = e.getComponent<ModelComp>();
        auto &tc = e.getComponent<TransformComp>();

        /*glm::quat rot{
            glm::angleAxis(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)) *
            glm::angleAxis(glm::radians(30.f), glm::vec3(0.f, 0.f, 1.f)) *
            glm::angleAxis(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f))};*/

        m_Renderer->drawModel(modelComp.id, tc);
    }
}

void Scene::onUIRender() {
    ImGui::Begin("Scene UI", &m_IsRunning);
    ImGui::SetWindowPos(ImVec2(0, 0));
    ImGui::SetWindowSize(ImVec2(200, 600));

    auto inspectTransform = [](TransformComp& tc) {
        ImGui::DragFloat3("Position", glm::value_ptr(tc.position), 0.1f);
        if (ImGui::DragFloat3("Rotation", glm::value_ptr(tc.eulerAngles), 0.1f)) {
            tc.rotation = glm::quat(glm::radians(tc.eulerAngles));
        }
        ImGui::DragFloat3("Scale", glm::value_ptr(tc.scale), 0.1f);
    };

    ImGui::BeginGroup();
    for (auto &e : getEntities<TransformComp>()) {
        ImGui::PushID(e.getComponent<UUIDComp>().id);
        //Have dropdown of all entities
        if (ImGui::TreeNode(e.getComponent<TagComp>().tag.c_str())) {
            if (ImGui::CollapsingHeader("Transform")) {
                auto& tc = e.getComponent<TransformComp>();
                inspectTransform(tc);
            }

            ImGui::TreePop();
        }
        ImGui::PopID();

    }
    ImGui::EndGroup();

    ImGui::End();
}

void Scene::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    m_Window = ctx->window.get();

    std::optional<SYN::InputContextHandle> gameplayHandle{
        ctx->inputManager.get()->addInputContext(0)};

    InputContext *gameplayCtx{ctx->inputManager.get()
                                  ->getInputContext(gameplayHandle.value())
                                  .value()};

    gameplayCtx->setConsumesInput(false);
    gameplayCtx->bindActions(SYN::InputKey::W, {{Action::MoveForward, {}}});
    gameplayCtx->bindActions(SYN::InputKey::S, {{Action::MoveBackward, {}}});
    gameplayCtx->bindActions(SYN::InputKey::A, {{Action::MoveLeft, {}}});
    gameplayCtx->bindActions(SYN::InputKey::D, {{Action::MoveRight, {}}});
    gameplayCtx->bindActions(SYN::InputKey::Space, {{Action::MoveUp, {}}});
    gameplayCtx->bindActions(SYN::InputKey::LeftCtrl, {{Action::MoveDown, {}}});
    gameplayCtx->bindActions(SYN::InputKey::LeftShift, {{Action::Run, {}}});
    gameplayCtx->bindActions(SYN::RawInputType::MouseDelta,
                             {{Action::LookDelta, {}}});
    gameplayCtx->bindActions(SYN::InputKey::Escape,
                             {{Action::ToggleCursor, {}}});
    gameplayCtx->addVectorAxis("GroundMovement",
                               {Action::MoveForward, Action::MoveBackward,
                                Action::MoveRight, Action::MoveLeft});

    gameplayCtx->addVectorAxis("FlyMovement",
                               {Action::MoveUp, Action::MoveDown});

    gameplayCtx->addVectorAxisCallback("FlyMovement", [&](float x, float y) {
        m_Dy = -y;
        spdlog::info("mdy = {}", m_Dy);
    });

    gameplayCtx->addVectorAxisCallback("GroundMovement", [&](float x, float z) {
        m_Dx = -x;
        m_Dz = z;
    });

    gameplayCtx->addActionCallback(Action::ToggleCursor,
                                   [&](InputState inputState) {
                                       if (inputState == InputState::Up) {
                                           return;
                                       }

                                       if (m_CursorHidden) {
                                           m_Window->enableCursor();
                                       } else {
                                           m_Window->disableCursor();
                                       }
                                       m_CursorHidden = !m_CursorHidden;
                                   });
    gameplayCtx->addActionCallback(Action::Run, [&](InputState inputState) {
        if (inputState == InputState::Down) {
            m_IsRunning = true;
        } else {
            m_IsRunning = false;
        }
    });

    gameplayCtx->addActionCallback(Action::LookDelta,
                                   [&](double dx, double dy) {
                                       m_DxRot -= dx;
                                       m_DyRot += dy;
                                   });

    auto e = createEntity();
    UUID uuid{ctx->projectConfig.assetManager->loadModel(
        "resources/assets/Cabin/scene.gltf")};
    spdlog::info("uuid = {}", uuid);

    e.addComponent<ModelComp>(ModelComp{.id = uuid});

    auto cam = createEntity("Primary Camera");
    auto& tc = cam.getComponent<TransformComp>();
    tc.position = glm::vec3(0.f, 0.f, -2.f);
    tc.rotation = glm::quat(0.f, 0.f, 0.f, 0.f);
    tc.scale = glm::vec3(1.f, 1.f, 1.f);

    auto& c = cam.addComponent<CameraComp>();
    c.fovDegrees = 90.f;
    c.aspectRatio = 16.f / 9.f;
    c.nearPlane = 0.0001;
    c.farPlane = 100.f;
    c.isPrimary = true;
}

Entity Scene::createEntity(const std::string &tag) {
    auto ent = Entity(&m_SceneState, m_SceneState.registry.create());
    ent.addComponent<UUIDComp>(generateUUID());
    ent.addComponent<TagComp>(TagComp{.tag = tag});
    ent.addComponent<TransformComp>();
    return ent;
}
