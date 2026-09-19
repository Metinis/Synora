#pragma once

#include <SynoraEngine/renderer/DebugDraw.h>
#include <SynoraEngine/renderer/types/RenderEffectDesc.h>
#include <SynoraEngine/scene/view/RenderView3D.h>

#include <SynoraEngine/renderer/types/DrawOptions.h>

#include <imgui.h>

namespace SYN {
// This interface takes in backend neutral scene data and decides
// how to render it. This is not meant to abstract a graphics API,
// but instead a full renderer which may come in any form.

class IRenderViewBackend {
  public:
    IRenderViewBackend() = default;
    virtual ~IRenderViewBackend() {}

    virtual void init(class EngineContext *context) {}
    virtual void shutdown() {}

    virtual void beforeDraw() {}
    virtual void afterDraw() {}

    virtual std::optional<ImTextureID>
    getHandleForImGui(UUID renderTarget,
                      std::optional<uint32_t> attachmentIndex) {
        return std::nullopt;
    };

    virtual void createShader(std::filesystem::path shaderPath,
                              const std::string &key) = 0;
    virtual void createEffect(const RenderEffectDesc &desc) = 0;

    virtual void beginFrame(const RenderView3D &sceneDescription) = 0;

    // if renderTarget is not specified then draw to default framebuffer
    // If effect is not specified then use default internal multi-pass
    virtual void draw(const DrawOptions &options) = 0;

    virtual void endFrame() = 0;

    virtual void submitLineList(const std::vector<DebugDraw::Line> &lines,
                                bool depthTest) {};
};
} // namespace SYN
