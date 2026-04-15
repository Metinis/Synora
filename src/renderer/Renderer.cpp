#include "Renderer.h"
#include "PuzzleEngine/core/Window.h"
#include "RenderGraph.h"
#include "backends/vulkan/Backend.h"
#include "glm/ext/quaternion_common.hpp"
#include "render_passes/FirstPass.h"
#include "render_passes/ImGUIPass.h"
#include "render_passes/SkyBoxPass.h"
#include "renderer/RenderTypes.h"

using namespace SYN;

namespace {
SYN::TextureData loadTexture(const std::string &path);
}

void SYN::Renderer::init(EngineContext *ctx) {
    m_Backend = std::make_unique<VK::VulkanBackend>();
    m_Backend->init(ctx->window.get());
    m_Window = ctx->window.get();

    m_MSAADepthAttachment = m_Backend->createAttachment(
        AttachmentDesc{.type = TextureType::depth, .msaaSamples = 4});

    m_MSAAScreenColorAttachment = m_Backend->createAttachment(AttachmentDesc{
        .type = TextureType::srgb,
        .msaaSamples = 4,
    });

    TextureData right{loadTexture("resources/assets/SunsetSkybox/right.png")};
    TextureData left{loadTexture("resources/assets/SunsetSkybox/left.png")};
    TextureData up{loadTexture("resources/assets/SunsetSkybox/up.png")};
    TextureData down{loadTexture("resources/assets/SunsetSkybox/down.png")};
    TextureData front{loadTexture("resources/assets/SunsetSkybox/front.png")};
    TextureData back{loadTexture("resources/assets/SunsetSkybox/back.png")};

    TextureDesc skyboxDesc{
        .width = right.width,
        .height = right.height,
        .layerCount = 6,
        .type = TextureType::srgb,
        .hasMipChain = false,
        .isCubeMap = true,
    };

    m_SkyBox = m_Backend->createTexture(skyboxDesc);
    std::array<const void *, 6> faces{right.data, left.data,  up.data,
                                      down.data,  front.data, back.data};

    m_Backend->uploadToTexture(m_SkyBox, faces, right.width, right.height);

    stbi_image_free(right.data);
    stbi_image_free(left.data);
    stbi_image_free(up.data);
    stbi_image_free(down.data);
    stbi_image_free(front.data);
    stbi_image_free(back.data);
}

void SYN::Renderer::render(Window &window) {
    AttachmentHandle swapchainAttachment{
        m_Backend->getSwapchainAttachmentCmd()};

    uint32_t msaaSamples{4};
    m_RenderGraph.addPass<FirstPass>(
        msaaSamples, m_DrawCalls, m_CurrentCameraProjection,
        m_CurrentCameraView, m_MSAAScreenColorAttachment, m_MSAADepthAttachment,
        swapchainAttachment);

    m_RenderGraph.addPass<SkyBoxPass>(
        msaaSamples, m_CurrentCameraProjection, m_CurrentCameraView, m_SkyBox,
        m_MSAAScreenColorAttachment, m_MSAADepthAttachment,
        swapchainAttachment);

    m_RenderGraph.addPass<ImGUIPass>(swapchainAttachment);
    m_RenderGraph.compile(*m_Backend);

    m_Backend->beginFrame(*m_Window);
    m_RenderGraph.execute(*m_Backend);
    m_Backend->endFrame(*m_Window);

    m_DrawCalls.clear();
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
        auto &model = m_UploadedModels[modelID];
        for (auto &mesh : model.meshes) {
            m_Backend->destroyTexture(mesh.albedo);
            m_Backend->destroyBuffer(mesh.vertexBuffer);
            m_Backend->destroyBuffer(mesh.indexBuffer);
        }
        m_UploadedModels.erase(modelID);
        spdlog::debug("Removed model from renderer {}", modelID);
    } else {
        spdlog::warn("Model (uuid = {}) does not exist in Uploaded Models",
                     modelID);
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
    if (!m_UploadedModels.contains(modelID)) {
        spdlog::warn(
            "Trying to draw model (uuid = {}) that was not added to renderer",
            modelID);
        return;
    }
    glm::mat4 translation{glm::translate(glm::mat4(1.0f), transform.position)};
    glm::mat4 rotation{glm::mat4(transform.rotation)};
    glm::mat4 scale{glm::scale(glm::mat4(1.0f), transform.scale)};

    glm::mat4 modelMat{translation * rotation * scale};

    UploadedModel &model{m_UploadedModels[modelID]};
    for (auto &mesh : model.meshes) {
        m_DrawCalls.emplace_back(MeshDrawCall{
            .mesh = mesh,
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
    m_Backend->destroyAttachment(m_MSAAScreenColorAttachment);
    m_Backend->destroyAttachment(m_MSAADepthAttachment);
    m_Backend->destroyTexture(m_SkyBox);

    m_Backend->shutdown();
}

namespace {
SYN::TextureData loadTexture(const std::string &path) {
    spdlog::debug("Loading {}", path);

    int imageWidth{};
    int imageHeight{};
    int channelsInImage{};
    stbi_uc *imageBytes{stbi_load(path.c_str(), &imageWidth, &imageHeight,
                                  &channelsInImage, 4)};

    assert(imageWidth >= 0);
    assert(imageHeight >= 0);
    if (imageBytes == nullptr) {
        spdlog::warn("Could not load {}", path);
        return {};
    }

    return TextureData{
        .width = static_cast<uint32_t>(imageWidth),
        .height = static_cast<uint32_t>(imageHeight),
        .data = imageBytes,
    };
}
} // namespace
