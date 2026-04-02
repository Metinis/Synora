#include "PuzzleEngine/scene/Scene.h"

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
    glm::vec3 lookDir{m_Camera.transform.rotation * glm::vec3(0.f, 0.f, 1.f)};
    lookDir.y = 0.f;
    lookDir = glm::normalize(lookDir);

    glm::vec3 upDir(0.f, 1.f, 0.f);
    glm::vec3 sideDir{glm::cross(lookDir, upDir)};

    glm::vec3 dPos{(lookDir * m_Dz) + (sideDir * m_Dx) + (upDir * -m_Dy)};
    dPos *= dt * speed;

    m_Camera.transform.position += dPos;

    float sensitivity{0.1f};
    glm::quat yaw{
        glm::angleAxis(glm::radians(static_cast<float>(-m_DxRot * sensitivity)),
                       glm::vec3(0.f, 1.f, 0.f))};
    glm::quat pitch{
        glm::angleAxis(glm::radians(static_cast<float>(m_DyRot * sensitivity)),
                       glm::vec3(1.f, 0.f, 0.f))};

    m_Camera.transform.rotation =
        glm::normalize(yaw * m_Camera.transform.rotation * pitch);

    m_DyRot = 0;
    m_DxRot = 0;
}

void Scene::onAttach() { spdlog::debug("Scene: Attached"); }

void Scene::onDettach() { spdlog::debug("Scene: Dettached"); }

void Scene::onRender() {
    m_Renderer->setCamera(m_Camera);
    for (auto &e : getEntities<ModelComp>()) {
        auto &modelComp = e.getComponent<ModelComp>();

        glm::quat rot{
            glm::angleAxis(glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)) *
            glm::angleAxis(glm::radians(30.f), glm::vec3(0.f, 0.f, 1.f)) *
            glm::angleAxis(glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f))};

        Transform pos{.rotation = rot};
        m_Renderer->drawModel(modelComp.id, pos);
    }
}

void Scene::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    m_Window = ctx->window.get();
    m_Camera = Camera{
        .transform =
            {
                .position = glm::vec3(0.f, 0.f, -2.f),
                .rotation = glm::quat(0.f, 0.f, 0.f, 1.f),
                .scale = glm::vec3(1.f, 1.f, 1.f),
            },
        .fovDegrees = 90.f,
        .aspectRatio = 16.f / 9.f,
        .nearPlane = 0.0001,
        .farPlane = 100.f,
    };

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
        m_Dy = y;
        spdlog::info("mdy = {}", m_Dy);
    });

    gameplayCtx->addVectorAxisCallback("GroundMovement", [&](float x, float z) {
        m_Dx = x;
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
                                       m_DxRot += dx;
                                       m_DyRot += dy;
                                   });

    auto e = createEntity();
    UUID uuid{ctx->projectConfig.assetManager->loadModel(
        "resources/assets/Test/test.obj")};
    spdlog::info("uuid = {}", uuid);

    e.addComponent<ModelComp>(ModelComp{.id = uuid});
}

Entity Scene::createEntity() {
    return Entity(&m_SceneState, m_SceneState.registry.create());
}
