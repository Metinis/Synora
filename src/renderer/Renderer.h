#pragma once
#include "PuzzleEngine/core/Application.h"
#include "RenderGraph.h"
#include "backends/IBackend.h"
#include "renderer/RenderTypes.h"
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>

namespace SYN {
class Window;

class Renderer {
  public:
    Renderer() = default;
    ~Renderer() = default;

    void init(EngineContext *ctx);
    void render(Window &window);
    void addMesh(UUID meshID, const MeshData &meshData);
    // could make a renderable object struct with meshID, materialID etc, all
    // that are needed for drawing
    void drawMesh(UUID meshID);
    void shutdown();

  private:
    std::unique_ptr<IBackend> m_Backend;
    Window *m_Window;
    RenderGraph m_RenderGraph;

    AttachmentHandle m_DepthAttachment;
    std::unordered_map<UUID, TextureHandle> m_Textures;
    std::unordered_map<UUID, BufferHandle> m_Buffers;
};

} // namespace SYN
