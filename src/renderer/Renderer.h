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
    void addModel(UUID modelID, const ModelData &modelData);

    // could make a renderable object struct with modelID, materialID etc, all
    // that are needed for drawing
    void drawModel(UUID modelID, const glm::vec3 &pos);
    void shutdown();

    struct UploadedMesh {
        BufferHandle vertexBuffer;
        BufferHandle indexBuffer;
        glm::mat4 localTransform;
        size_t numIndices;

        TextureHandle albedo;
    };

    struct UploadedModel {
        std::vector<UploadedMesh> meshes;
    };
    struct MeshDrawCall {
        UploadedMesh *mesh;
        glm::vec3 pos;
    };

  private:
    std::unique_ptr<IBackend> m_Backend;
    Window *m_Window;
    RenderGraph m_RenderGraph;

    AttachmentHandle m_DepthAttachment;

    std::unordered_map<UUID, UploadedModel> m_UploadedModels;
    std::vector<MeshDrawCall> m_DrawCalls;
};

} // namespace SYN
