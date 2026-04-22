#include "SynoraEngine/renderer/Renderer.h"
#include "RenderGraph.h"
#include "SynoraEngine/core/Window.h"
#include "SynoraEngine/renderer/RenderTypes.h"
#include "glm/ext/quaternion_common.hpp"
#include "render_passes/ImGUIPass.h"
#include "render_passes/LightingPass.h"
#include "render_passes/SkyBoxPass.h"
#include <memory>
#include <vulkan/vulkan_core.h>

#include "backends/vulkan/Device.h"

using namespace SYN;

namespace {
SYN::TextureData loadTexture(const std::string &path);
}

SYN::Renderer::Renderer() = default;
SYN::Renderer::~Renderer() = default;

void SYN::Renderer::init(EngineContext *ctx) {
    m_Device = std::make_unique<VK::VulkanDevice>();
    m_Device->init(ctx->window.get());

    m_GraphicsCtx = m_Device->makeGraphicsContext();

    m_RenderGraph = std::make_unique<RenderGraph>();

    m_Window = ctx->window.get();

    m_MSAADepthAttachment = m_Device->createAttachment(
        AttachmentDesc{.type = TextureType::depth, .msaaSamples = 4});

    m_MSAAScreenColorAttachment = m_Device->createAttachment(AttachmentDesc{
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
        .type = TextureType::srgb,
        .hasMipChain = false,
        .isCubeMap = true,
    };

    m_SkyBox = m_Device->createTexture(skyboxDesc);
    std::array<const void *, 6> faces{right.data, left.data,  up.data,
                                      down.data,  front.data, back.data};

    m_GraphicsCtx->uploadToTexture(m_SkyBox, faces, right.width, right.height);

    stbi_image_free(right.data);
    stbi_image_free(left.data);
    stbi_image_free(up.data);
    stbi_image_free(down.data);
    stbi_image_free(front.data);
    stbi_image_free(back.data);
}

void SYN::Renderer::render(Window &window) {
    m_GraphicsCtx->beginFrame(*m_Window);

    AttachmentHandle swapchainAttachment{
        m_GraphicsCtx->acquireSwapchainAttachmentCmd()};

    uint32_t msaaSamples{4};
    m_RenderGraph->addPass<LightingPass>(
        msaaSamples, m_DrawCalls, m_CurrentCameraProjection,
        m_CurrentCameraView, m_MSAAScreenColorAttachment, m_MSAADepthAttachment,
        swapchainAttachment);

    m_RenderGraph->addPass<SkyBoxPass>(
        msaaSamples, m_CurrentCameraProjection, m_CurrentCameraView, m_SkyBox,
        m_MSAAScreenColorAttachment, m_MSAADepthAttachment,
        swapchainAttachment);

    m_RenderGraph->addPass<ImGUIPass>(swapchainAttachment);
    m_RenderGraph->compile(*m_Device);

    m_RenderGraph->execute(*m_GraphicsCtx);

    m_GraphicsCtx->endFrame(*m_Window);

    m_DrawCalls.clear();
}

void Renderer::addMesh(UUID modelID, const MeshData &meshData) {
    if (m_UploadedMeshes.contains(modelID)) {
        spdlog::warn("Trying to add model (uuid = {}) that was already added",
                     modelID);
        return;
    }

    BufferHandle vertexBuffer{m_Device->createBuffer(
        {.size = meshData.vertices.size() * sizeof(Vertex)})};

    BufferHandle indexBuffer{m_Device->createBuffer(
        {.size = meshData.indices.size() * sizeof(uint32_t)})};

    m_GraphicsCtx->uploadToBuffer(vertexBuffer,
                                  meshData.vertices.size() * sizeof(Vertex),
                                  meshData.vertices.data());
    m_GraphicsCtx->uploadToBuffer(indexBuffer,
                                  meshData.indices.size() * sizeof(uint32_t),
                                  meshData.indices.data());

    TextureHandle albedo{
        m_Device->createTexture({.width = meshData.albedo->width,
                                 .height = meshData.albedo->height,
                                 .type = TextureType::srgb})};
    m_GraphicsCtx->uploadToTexture(albedo, meshData.albedo->data,
                                   meshData.albedo->width,
                                   meshData.albedo->height);

    TextureHandle metallicRoughness{
        m_Device->createTexture({.width = meshData.metallicRoughness->width,
                                 .height = meshData.metallicRoughness->height,
                                 .type = TextureType::rgba})};
    m_GraphicsCtx->uploadToTexture(
        metallicRoughness, meshData.metallicRoughness->data,
        meshData.metallicRoughness->width, meshData.metallicRoughness->height);

    TextureHandle normalMap{
        m_Device->createTexture({.width = meshData.normalMap->width,
                                 .height = meshData.normalMap->height,
                                 .type = TextureType::rgba})};
    m_GraphicsCtx->uploadToTexture(normalMap, meshData.normalMap->data,
                                   meshData.normalMap->width,
                                   meshData.normalMap->height);

    auto mesh = UploadedMesh{.vertexBuffer = vertexBuffer,
                             .indexBuffer = indexBuffer,
                             .localTransform = meshData.localTransform,
                             .numIndices = meshData.indices.size(),
                             .albedo = albedo,
                             .metallicRoughness = metallicRoughness,
                             .normalMap = normalMap};
    m_UploadedMeshes[modelID] = std::move(mesh);
    spdlog::info("added model");
}

void Renderer::removeMesh(UUID meshID) {
    if (m_UploadedMeshes.contains(meshID)) {
        auto &mesh = m_UploadedMeshes[meshID];
        m_Device->destroyTexture(mesh.albedo);
        m_Device->destroyTexture(mesh.metallicRoughness);
        m_Device->destroyTexture(mesh.normalMap);
        m_Device->destroyBuffer(mesh.vertexBuffer);
        m_Device->destroyBuffer(mesh.indexBuffer);
        m_UploadedMeshes.erase(meshID);
        spdlog::debug("Removed model from renderer {}", meshID);
    } else {
        spdlog::warn("Model (uuid = {}) does not exist in Uploaded Models",
                     meshID);
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

void Renderer::drawMesh(UUID modelID, const glm::mat4 &worldMatrix) {
    auto it{m_UploadedMeshes.find(modelID)};
    if (it == m_UploadedMeshes.end()) {
        spdlog::warn(
            "Trying to draw model (uuid = {}) that was not added to renderer",
            modelID);
        return;
    }

    UploadedMesh &mesh{it->second};
    m_DrawCalls.emplace_back(MeshDrawCall{
        .mesh = mesh,
        .modelMatrix = worldMatrix * mesh.localTransform,
    });
}

void SYN::Renderer::shutdown() {
    m_RenderGraph->shutdown(*m_Device);
    for (auto &[uuid, mesh] : m_UploadedMeshes) {
        m_Device->destroyTexture(mesh.albedo);
        m_Device->destroyTexture(mesh.metallicRoughness);
        m_Device->destroyTexture(mesh.normalMap);
        m_Device->destroyBuffer(mesh.vertexBuffer);
        m_Device->destroyBuffer(mesh.indexBuffer);
    }
    m_Device->destroyAttachment(m_MSAAScreenColorAttachment);
    m_Device->destroyAttachment(m_MSAADepthAttachment);
    m_Device->destroyTexture(m_SkyBox);

    m_Device->shutdown();
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
