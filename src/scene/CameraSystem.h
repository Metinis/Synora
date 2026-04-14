#pragma once
#include "PuzzleEngine/core/Application.h"
#include "PuzzleEngine/scene/Entity.h"

namespace SYN {
    struct Camera;

    class CameraSystem : public ILayer {
    public:
        CameraSystem();
        Entity getCameraEntity();
        Camera getCamera();
        void onAttach() override;
        void onDettach() override;
        void onUpdate(float dt) override;
        void onRender() override;
        void onUIRender() override;
        void init(EngineContext* ctx);
        ~CameraSystem() = default;
    private:
        Scene *m_Scene;
        Renderer *m_Renderer;
        Window *m_Window;
        float m_Dx;
        float m_Dy;
        float m_Dz;
        float m_DxRot;
        float m_DyRot;
        bool m_CursorHidden{};
        bool m_LastCursorHiddenState{}; // used to prevent camera from jumping on
        // hitting esc

        float m_WalkSpeed{1.f};
        float m_RunMultiplier{2.f};
        bool m_IsRunning{};
    };
}
