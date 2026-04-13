#include "Renderer.h"
#include "PuzzleEngine/core/Window.h"
#include "RenderGraph.h"
#include "backends/vulkan/Backend.h"
#include "glm/ext/quaternion_common.hpp"
#include "render_passes/FirstPass.h"
#include "renderer/RenderTypes.h"
#include "render_passes/ImGUIPass.h"

using namespace SYN;

void SYN::Renderer::init(EngineContext *ctx) {
    m_Backend = std::make_unique<VK::VulkanBackend>();
    m_Backend->init(ctx->window.get());
    m_Window = ctx->window.get();

    m_DepthAttachment =
        m_Backend->createAttachment(AttachmentDesc{.type = TextureType::depth});
}

void SYN::Renderer::render(Window &window) {
    AttachmentHandle swapchainAttachment{
        m_Backend->getSwapchainAttachmentCmd()};

    m_RenderGraph.addPass<FirstPass>(m_DrawCalls, m_CurrentCameraProjection,
                                     m_CurrentCameraView, swapchainAttachment,
                                     m_DepthAttachment);
    m_RenderGraph.addPass<ImGUIPass>();

    m_RenderGraph.compile(*m_Backend);

    m_Backend->beginFrame(*m_Window);
    m_RenderGraph.execute(*m_Backend);
    m_Backend->endFrame(*m_Window);

    m_DrawCalls.clear();

    for (auto& f : m_RemoveQueue) {
        f();
    }
    m_RemoveQueue.clear();
}

void Renderer::addModel(UUID modelID, const ModelData &modelData) {
    if (m_UploadedModels.contains(modelID)) {
        spdlog::warn("Trying to add model (uuid = {}) that was already added",
                     modelID);
        return;
    }

    UploadedModel model{};

    for (const auto &mesh : modelData.meshes) {
        TextureHandle albedo{
            m_Backend->uploadTexture({.width = mesh.albedo->width,
                                      .height = mesh.albedo->height,
                                      .type = TextureType::srgb},
                                     mesh.albedo->data)};

        BufferHandle vertexBuffer{m_Backend->uploadBuffer(
            {.size = mesh.vertices.size() * sizeof(Vertex)},
            mesh.vertices.data())};

        BufferHandle indexBuffer{m_Backend->uploadBuffer(
            {.size = mesh.indices.size() * sizeof(uint32_t)},
            mesh.indices.data())};

        model.meshes.emplace_back(UploadedMesh{
            .vertexBuffer = vertexBuffer,
            .indexBuffer = indexBuffer,
            .localTransform = mesh.localTransform,
            .numIndices = mesh.indices.size(),
            .albedo = albedo,
        });
    }
    m_UploadedModels[modelID] = std::move(model);
    spdlog::info("added model");
}

void Renderer::removeModel(UUID modelID) {
    if (m_UploadedModels.contains(modelID)) {
        auto& model = m_UploadedModels[modelID];
        for (auto &mesh : model.meshes) {
            m_RemoveQueue.push_back([this, mesh] {
                m_Backend->destroyTexture(mesh.albedo);
                m_Backend->destroyBuffer(mesh.vertexBuffer);
                m_Backend->destroyBuffer(mesh.indexBuffer);
            });

        }
        m_RemoveQueue.push_back([this, modelID] {
            m_UploadedModels.erase(modelID);
        });
        spdlog::debug("Removed model from renderer {}", modelID);
    } else {
        spdlog::warn("Model (uuid = {}) does not exist in Uploaded Models", modelID);
    }
}

void Renderer::setCamera(const Camera &camera) {
    m_CurrentCameraProjection =
        glm::perspective(glm::radians(camera.fovDegrees), camera.aspectRatio,
                         camera.nearPlane, camera.farPlane);

    glm::mat4 rotation(glm::conjugate(camera.transform.rotation));
    glm::mat4 translation{
        glm::translate(glm::mat4(1.f), -camera.transform.position)};

    m_CurrentCameraView = rotation * translation;
}

void Renderer::drawModel(UUID modelID, const TransformComp &transform) {
    auto it{m_UploadedModels.find(modelID)};
    if (it == m_UploadedModels.end()) {
        spdlog::warn(
            "Trying to draw model (uuid = {}) that was not added to renderer",
            modelID);
        return;
    }
    glm::mat4 translation{glm::translate(glm::mat4(1.0f), transform.position)};
    glm::mat4 rotation{glm::mat4(transform.rotation)};
    glm::mat4 scale{glm::scale(glm::mat4(1.0f), transform.scale)};

    glm::mat4 modelMat{translation * rotation * scale};

    UploadedModel &model{it->second};
    for (auto &mesh : model.meshes) {
        m_DrawCalls.emplace_back(MeshDrawCall{
            .mesh = &mesh,
            .modelMatrix = modelMat * mesh.localTransform,
        });
    }
}

void SYN::Renderer::shutdown() {
    m_RenderGraph.shutdown(*m_Backend);
    for (auto &[uuid, model] : m_UploadedModels) {
        for (auto &mesh : model.meshes) {
            m_Backend->destroyTexture(mesh.albedo);
            m_Backend->destroyBuffer(mesh.vertexBuffer);
            m_Backend->destroyBuffer(mesh.indexBuffer);
        }
    }
    m_Backend->destroyAttachment(m_DepthAttachment);

    m_Backend->shutdown();
}
