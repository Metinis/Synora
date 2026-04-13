#pragma once
#include "PuzzleEngine/core/Application.h"
#include "RenderGraph.h"
#include "backends/IBackend.h"
#include "renderer/RenderTypes.h"
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>

#include "PuzzleEngine/scene/Components.h"

namespace SYN {
class Window;

struct Camera {
    TransformComp transform;
    float fovDegrees;
    float aspectRatio;
    float nearPlane;
    float farPlane;
};

class Renderer {
  public:
    Renderer() = default;
    ~Renderer() = default;

    void init(EngineContext *ctx);
    void render(Window &window);
    void addModel(UUID modelID, const ModelData &modelData);
    void removeModel(UUID modelID);

    void setCamera(const Camera &camera);

    // could make a renderable object struct with modelID, materialID etc, all
    // that are needed for drawing
    void drawModel(UUID modelID, const TransformComp &transform);
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
        glm::mat4 modelMatrix;
    };

  private:
    // TODO: change this, this is for testing / demoing
    glm::mat4 m_CurrentCameraProjection;
    glm::mat4 m_CurrentCameraView;

    std::unique_ptr<IBackend> m_Backend;
    Window *m_Window;
    RenderGraph m_RenderGraph;

    AttachmentHandle m_DepthAttachment;

    std::unordered_map<UUID, UploadedModel> m_UploadedModels;
    std::vector<MeshDrawCall> m_DrawCalls;
    std::vector<std::function<void()>> m_RemoveQueue;
};

} // namespace SYN
