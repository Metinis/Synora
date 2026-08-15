#include "../../include/SynoraEngine/renderer/Renderer.h"
#include "../../include/SynoraEngine/renderer/RenderTypes.h"
#include "RenderGraph.h"
#include "SynoraEngine/core/Window.h"
#include "backends/vulkan/Backend.h"
#include "glm/ext/quaternion_common.hpp"
#include "render_passes/ImGUIPass.h"
#include "render_passes/LightingPass.h"
#include "render_passes/SkyBoxPass.h"

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
    m_RenderGraph.addPass<LightingPass>(
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

void Renderer::addMesh(UUID modelID, const MeshData &meshData) {
    if (m_UploadedMeshes.contains(modelID)) {
        spdlog::warn("Trying to add model (uuid = {}) that was already added",
                     modelID);
        return;
    }

    BufferHandle vertexBuffer{m_Backend->uploadBuffer(
        {.size = meshData.vertices.size() * sizeof(Vertex)},
        meshData.vertices.data())};

    BufferHandle indexBuffer{m_Backend->uploadBuffer(
        {.size = meshData.indices.size() * sizeof(uint32_t)},
        meshData.indices.data())};

    auto mesh = UploadedMesh{.vertexBuffer = vertexBuffer,
                             .indexBuffer = indexBuffer,
                             .numIndices = meshData.indices.size()};



    m_UploadedMeshes[modelID] = std::move(mesh);
}
void Renderer::addMaterial(UUID materialID, const MaterialData &materialData) {
    if (m_UploadedMaterials.contains(materialID)) {
        spdlog::warn("Trying to add material (uuid = {}) that was already added",
                     materialID);
        return;
    }

    UploadedMaterial mat{};

    if (materialData.albedo) {
        TextureHandle albedo{
            m_Backend->uploadTexture({.width = materialData.albedo->width,
                                      .height = materialData.albedo->height,
                                      .type = TextureType::srgb},
                                     materialData.albedo->data)};
        mat.albedo = albedo;
    }
    if (materialData.metallicRoughness) {
        TextureHandle metallicRoughness{
            m_Backend->uploadTexture({.width = materialData.metallicRoughness->width,
                                      .height = materialData.metallicRoughness->height,
                                      .type = TextureType::rgba},
                                     materialData.metallicRoughness->data)};
        mat.metallicRoughness = metallicRoughness;
    }
    if (materialData.normalMap) {
        TextureHandle normalMap{
            m_Backend->uploadTexture({.width = materialData.normalMap->width,
                                      .height = materialData.normalMap->height,
                                      .type = TextureType::rgba},
                                     materialData.normalMap->data)};
        mat.normalMap = normalMap;
    }
    m_UploadedMaterials[materialID] = std::move(mat);
}
void Renderer::removeMaterial(UUID materialID) {
    if (m_UploadedMaterials.contains(materialID)) {
        auto &mat = m_UploadedMaterials[materialID];
        m_Backend->destroyTexture(mat.albedo);
        m_Backend->destroyTexture(mat.metallicRoughness);
        m_Backend->destroyTexture(mat.normalMap);
        m_UploadedMaterials.erase(materialID);
        //spdlog::debug("Removed material from renderer {}", materialID);
    } else {
        spdlog::warn("Material (uuid = {}) does not exist in Uploaded Models",
                     materialID);
    }
}

void Renderer::removeMesh(UUID meshID) {
    if (m_UploadedMeshes.contains(meshID)) {
        auto &mesh = m_UploadedMeshes[meshID];

        m_Backend->destroyBuffer(mesh.vertexBuffer);
        m_Backend->destroyBuffer(mesh.indexBuffer);
        m_UploadedMeshes.erase(meshID);
        //spdlog::debug("Removed model from renderer {}", meshID);
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

void Renderer::drawMesh(UUID meshID, UUID materialID, const glm::mat4 &worldMatrix) {
    auto itMesh{m_UploadedMeshes.find(meshID)};
    if (meshID == 0) {
        //empty mesh comp
        return;
    }
    if (itMesh == m_UploadedMeshes.end()) {
        //spdlog::warn(
        //    "Trying to draw model (uuid = {}) that was not added to renderer",
        //    meshID);
        return;
    }

    auto itMat{m_UploadedMaterials.find(materialID)};
    if (materialID == 0) {
        //empty mesh comp
        return;
    }
    if (itMat == m_UploadedMaterials.end()) {
        //spdlog::warn(
        //    "Trying to draw with material (uuid = {}) that was not added to renderer",
        //    materialID);
        return;
    }

    UploadedMesh &mesh{itMesh->second};
    UploadedMaterial &material{itMat->second};
    m_DrawCalls.emplace_back(DrawCall{
        .mesh = mesh,
        .material = material,
        .modelMatrix = worldMatrix,
    });
}

void SYN::Renderer::shutdown() {
    m_RenderGraph.shutdown(*m_Backend);
    for (auto &[uuid, mesh] : m_UploadedMeshes) {
        m_Backend->destroyBuffer(mesh.vertexBuffer);
        m_Backend->destroyBuffer(mesh.indexBuffer);
    }
    for (auto &[uuid, mat] : m_UploadedMaterials) {
        m_Backend->destroyTexture(mat.albedo);
        m_Backend->destroyTexture(mat.metallicRoughness);
        m_Backend->destroyTexture(mat.normalMap);
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
