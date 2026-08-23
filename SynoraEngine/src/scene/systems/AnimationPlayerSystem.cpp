#include "AnimationPlayerSystem.h"

#include <SynoraEngine/core/Application.h>
#include <SynoraEngine/scene/SceneManager.h>
#include <SynoraEngine/scene/components/ModelComponent.h>
#include <SynoraEngine/scene/components/SkeletalAnimationComponent.h>

namespace SYN {
void AnimationPlayerSystem::init(EngineContext *context) {
    m_SceneManager = context->sceneManager.get();
}

void AnimationPlayerSystem::onAttach() {}

void AnimationPlayerSystem::onUpdate(float dt) {
    SceneHandle currentSceneHandle = m_SceneManager->getActiveScene();
    if (!m_SceneManager->isSceneValid(currentSceneHandle))
        return;
    Scene *scene = m_SceneManager->getSceneMut(currentSceneHandle);

    scene->forEach<ModelComponent, SkeletalAnimationComponent>(
        [dt](Entity entity, ModelComponent &model,
             SkeletalAnimationComponent &animation) {
            animation.player.update(model.model.uuid(), dt);
        });
}

void AnimationPlayerSystem::onRender() {}

void AnimationPlayerSystem::onUIRender() {}

void AnimationPlayerSystem::onDettach() {}
} // namespace SYN
