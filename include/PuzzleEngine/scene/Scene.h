#pragma once
#include <entt/entt.hpp>

#include "PuzzleEngine/core/Layer.h"
#include "PuzzleEngine/scene/Entity.h"
#include "renderer/IBackend.h"
#include "scene/SceneState.h"

namespace SYN {
    class AssetManager;

    //probably split scene into scene manager and scene contain like this
    class Scene : public ILayer {
    public:
        Scene();
        ~Scene() override = default;

        void onUpdate(float dt) override;
        void onAttach() override;
        void onDettach() override;
        void onRender() override;

        void init(IBackend* backend, AssetManager* assetManager);
        Entity createEntity();

        template<typename T>
        std::vector<Entity> getEntities() {
            std::vector<Entity> ret;
            for (auto& e : m_SceneState.registry.view<T>()) {
                ret.push_back(Entity(&m_SceneState, e));
            }
            return ret;
        }
    private:
        IBackend* m_Backend;
        SceneState m_SceneState;
        friend class Entity;
    };

}
