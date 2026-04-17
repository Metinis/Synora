#include "CameraSystem.h"
#include "SynoraEngine/core/InputTypes.h"
#include "SynoraEngine/core/Input.h"
#include "SynoraEngine/core/InputContext.h"
#include "SynoraEngine/core/Window.h"
#include "SynoraEngine/scene/Scene.h"
#include "spdlog/spdlog.h"
#include "renderer/Renderer.h"

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

SYN::CameraSystem::CameraSystem() {
}

SYN::Entity SYN::CameraSystem::getCameraEntity() {
    for (auto e : m_Scene->getEntities<CameraComp>()) {
        if (e.getComponent<CameraComp>().isPrimary) {
            return e;
        }
    }
    return Entity();
}

SYN::Camera SYN::CameraSystem::getCamera() {
    auto camEntity = getCameraEntity();

    if (!m_Scene->isValidEntity(camEntity)) {
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

void SYN::CameraSystem::onAttach() {
    spdlog::debug("Camera System: Attached");
}

void SYN::CameraSystem::onDettach() {
    spdlog::debug("Camera System: Detached");
}

void SYN::CameraSystem::onUIRender() {

}

void SYN::CameraSystem::onUpdate(float dt) {
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
    auto &camTC = camera.getComponent<TransformComp>();

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
                       glm::vec3(0.f, 1.f, 0.f))
    };
    glm::quat pitch{
        glm::angleAxis(glm::radians(static_cast<float>(m_DyRot * sensitivity)),
                       glm::vec3(1.f, 0.f, 0.f))
    };

    camTC.rotation =
            glm::normalize(yaw * camTC.rotation * pitch);

    m_DyRot = 0;
    m_DxRot = 0;
}

void SYN::CameraSystem::onRender() {
    m_Renderer->setCamera(getCamera());
}

void SYN::CameraSystem::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    m_Window = ctx->window.get();
    m_Scene = ctx->scene.get();

    std::optional<SYN::InputContextHandle> gameplayHandle{
        ctx->inputManager.get()->addInputContext(0)
    };

    InputContext *gameplayCtx{
        ctx->inputManager.get()
        ->getInputContext(gameplayHandle.value())
        .value()
    };

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
                               {
                                   Action::MoveForward, Action::MoveBackward,
                                   Action::MoveRight, Action::MoveLeft
                               });

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
}
