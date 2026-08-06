#include "model_loader.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>
#include <stb_image.h>

#include <algorithm>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

using TextureStorage = std::shared_ptr<std::vector<uint8_t>>;

struct CachedTexture {
    uint32_t width = 0;
    uint32_t height = 0;
    SYN::gfx::gl::TextureFormat format = SYN::gfx::gl::TextureFormat::RGBA8;
    TextureStorage storage;
};

std::unordered_map<std::string, CachedTexture> g_TextureCache;

glm::mat4 toGlmMatrix(const aiMatrix4x4 &m) {
    return {m.a1, m.b1, m.c1, m.d1, m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3, m.a4, m.b4, m.c4, m.d4};
}

std::string makeTextureCacheKey(const std::filesystem::path &modelPath,
                                const std::string &texturePath) {
    if (!texturePath.empty() && texturePath.front() == '*') {
        return modelPath.string() + "|" + texturePath;
    }

    std::filesystem::path resolvedPath =
        (modelPath.parent_path() / texturePath).lexically_normal();
    return resolvedPath.string();
}

SYN::gfx::gl::TextureFormat textureFormatFromChannelCount(int channels,
                                                          bool srgb = false) {
    switch (channels) {
    case 1:
        return SYN::gfx::gl::TextureFormat::R8;
    case 2:
        return SYN::gfx::gl::TextureFormat::RG8;
    case 3:
        return srgb ? SYN::gfx::gl::TextureFormat::SRGB
                    : SYN::gfx::gl::TextureFormat::RGB8;
    case 4:
    default:
        return srgb ? SYN::gfx::gl::TextureFormat::SRGBA
                    : SYN::gfx::gl::TextureFormat::RGBA8;
    }
}

std::optional<SYN::gfx::gl::TextureData>
loadExternalTexture(const std::filesystem::path &texturePath,
                    const std::string &cacheKey, bool srgb = false) {
    if (auto cached = g_TextureCache.find(cacheKey);
        cached != g_TextureCache.end()) {
        const CachedTexture &cachedTexture = cached->second;
        return SYN::gfx::gl::TextureData{
            .info = {.width = cachedTexture.width,
                     .height = cachedTexture.height,
                     .format = cachedTexture.format},
            .data = std::span<uint8_t>(cachedTexture.storage->data(),
                                       cachedTexture.storage->size()),
            .sourcePath = cacheKey};
    }

    int width{};
    int height{};
    int channels{};
    stbi_uc *pixels =
        stbi_load(texturePath.string().c_str(), &width, &height, &channels, 0);
    if (pixels == nullptr) {
        spdlog::warn("Could not load texture {}", texturePath.string());
        return std::nullopt;
    }

    auto storage = std::make_shared<std::vector<uint8_t>>();
    storage->assign(pixels, pixels + (width * height * channels));
    stbi_image_free(pixels);

    g_TextureCache[cacheKey] = CachedTexture{
        static_cast<uint32_t>(width), static_cast<uint32_t>(height),
        textureFormatFromChannelCount(channels, srgb), storage};

    return SYN::gfx::gl::TextureData{
        .info = {.width = static_cast<uint32_t>(width),
                 .height = static_cast<uint32_t>(height),
                 .format = textureFormatFromChannelCount(channels, srgb)},
        .data = std::span<uint8_t>(storage->data(), storage->size()),
        .sourcePath = cacheKey};
}

std::optional<SYN::gfx::gl::TextureData>
loadEmbeddedCompressedTexture(const aiTexture *texture,
                              const std::string &cacheKey, bool srgb = false) {
    if (auto cached = g_TextureCache.find(cacheKey);
        cached != g_TextureCache.end()) {
        const CachedTexture &cachedTexture = cached->second;
        return SYN::gfx::gl::TextureData{
            .info = {.width = cachedTexture.width,
                     .height = cachedTexture.height,
                     .format = cachedTexture.format},
            .data = std::span<uint8_t>(cachedTexture.storage->data(),
                                       cachedTexture.storage->size()),
            .sourcePath = cacheKey};
    }

    int width{};
    int height{};
    int channels{};
    stbi_uc *pixels = stbi_load_from_memory(
        reinterpret_cast<const stbi_uc *>(texture->pcData),
        static_cast<int>(texture->mWidth), &width, &height, &channels, 0);
    if (pixels == nullptr) {
        spdlog::warn("Could not decode embedded texture {}", cacheKey);
        return std::nullopt;
    }

    auto storage = std::make_shared<std::vector<uint8_t>>();
    storage->assign(pixels, pixels + (width * height * channels));
    stbi_image_free(pixels);

    g_TextureCache[cacheKey] = CachedTexture{
        static_cast<uint32_t>(width), static_cast<uint32_t>(height),
        textureFormatFromChannelCount(channels, srgb), storage};

    return SYN::gfx::gl::TextureData{
        .info = {.width = static_cast<uint32_t>(width),
                 .height = static_cast<uint32_t>(height),
                 .format = textureFormatFromChannelCount(channels, srgb)},
        .data = std::span<uint8_t>(storage->data(), storage->size()),
        .sourcePath = cacheKey};
}

std::optional<SYN::gfx::gl::TextureData>
loadEmbeddedTexture(const aiTexture *texture, const std::string &cacheKey,
                    bool srgb) {
    if (texture == nullptr) {
        return std::nullopt;
    }

    if (texture->mHeight == 0) {
        return loadEmbeddedCompressedTexture(texture, cacheKey, srgb);
    }

    if (auto cached = g_TextureCache.find(cacheKey);
        cached != g_TextureCache.end()) {
        const CachedTexture &cachedTexture = cached->second;
        return SYN::gfx::gl::TextureData{
            .info = {.width = cachedTexture.width,
                     .height = cachedTexture.height,
                     .format = cachedTexture.format},
            .data = std::span<uint8_t>(cachedTexture.storage->data(),
                                       cachedTexture.storage->size()),
            .sourcePath = cacheKey};
    }

    auto storage = std::make_shared<std::vector<uint8_t>>();
    storage->resize(texture->mWidth * texture->mHeight * 4);

    const aiTexel *pixels = reinterpret_cast<const aiTexel *>(texture->pcData);
    for (uint32_t i = 0; i < texture->mWidth * texture->mHeight; ++i) {
        (*storage)[i * 4 + 0] = pixels[i].r;
        (*storage)[i * 4 + 1] = pixels[i].g;
        (*storage)[i * 4 + 2] = pixels[i].b;
        (*storage)[i * 4 + 3] = pixels[i].a;
    }

    g_TextureCache[cacheKey] =
        CachedTexture{texture->mWidth, texture->mHeight,
                      srgb ? SYN::gfx::gl::TextureFormat::SRGBA
                           : SYN::gfx::gl::TextureFormat::RGBA8,
                      storage};

    return SYN::gfx::gl::TextureData{
        .info =
            {
                .width = texture->mWidth,
                .height = texture->mHeight,
                .format = srgb ? SYN::gfx::gl::TextureFormat::SRGBA
                               : SYN::gfx::gl::TextureFormat::RGBA8,
            },
        .data = std::span<uint8_t>(storage->data(), storage->size()),
        .sourcePath = cacheKey};
}

std::optional<SYN::gfx::gl::TextureData>
loadTextureForMaterial(const aiMaterial *material, const aiScene *scene,
                       const std::filesystem::path &modelPath,
                       aiTextureType textureType) {
    aiString relPath{};
    if (material->GetTexture(textureType, 0, &relPath) != AI_SUCCESS) {
        return std::nullopt;
    }

    std::string texturePath = relPath.C_Str();
    std::string cacheKey = makeTextureCacheKey(modelPath, texturePath);

    bool isTextureAlbedo = textureType == aiTextureType_BASE_COLOR ||
                           textureType == aiTextureType_DIFFUSE;

    if (!texturePath.empty() && texturePath.front() == '*') {
        const aiTexture *embeddedTexture =
            scene->GetEmbeddedTexture(texturePath.c_str());
        return loadEmbeddedTexture(embeddedTexture, cacheKey, isTextureAlbedo);
    }

    std::filesystem::path resolvedPath =
        (modelPath.parent_path() / texturePath).lexically_normal();
    return loadExternalTexture(resolvedPath, cacheKey, isTextureAlbedo);
}

void addBoneData(SYN::gfx::gl::Vertex &vertex, int boneIndex, float weight) {
    for (int i = 0; i < 4; ++i) {
        if (vertex.boneWeights[i] == 0.0f) {
            vertex.boneIndices[i] = boneIndex;
            vertex.boneWeights[i] = weight;
            return;
        }
    }

    int smallestWeightIndex = 0;
    for (int i = 1; i < 4; ++i) {
        if (vertex.boneWeights[i] < vertex.boneWeights[smallestWeightIndex]) {
            smallestWeightIndex = i;
        }
    }

    if (weight > vertex.boneWeights[smallestWeightIndex]) {
        vertex.boneIndices[smallestWeightIndex] = boneIndex;
        vertex.boneWeights[smallestWeightIndex] = weight;
    }
}

void normalizeBoneWeights(SYN::gfx::gl::Vertex &vertex) {
    float totalWeight = vertex.boneWeights.x + vertex.boneWeights.y +
                        vertex.boneWeights.z + vertex.boneWeights.w;
    if (totalWeight <= 0.0f) {
        return;
    }

    vertex.boneWeights /= totalWeight;
}

void applyBonesToMesh(SYN::gfx::gl::MeshData &meshData, const aiMesh *mesh,
                      std::unordered_map<std::string, uint32_t> &boneIndexMap,
                      uint32_t &nextBoneIndex) {
    if (!mesh->HasBones()) {
        meshData.hasSkin = false;
        return;
    }

    meshData.hasSkin = true;

    for (unsigned int boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx) {
        const aiBone *bone = mesh->mBones[boneIdx];
        const std::string boneName = bone->mName.C_Str();
        uint32_t mappedBoneIndex{};
        auto [it, inserted] = boneIndexMap.emplace(boneName, nextBoneIndex);
        if (inserted) {
            mappedBoneIndex = nextBoneIndex;
            ++nextBoneIndex;
        } else {
            mappedBoneIndex = it->second;
        }

        for (unsigned int weightIdx = 0; weightIdx < bone->mNumWeights;
             ++weightIdx) {
            const aiVertexWeight &weight = bone->mWeights[weightIdx];
            if (weight.mVertexId >= meshData.vertices.size()) {
                continue;
            }
            addBoneData(meshData.vertices[weight.mVertexId], mappedBoneIndex,
                        weight.mWeight);
        }
    }

    for (SYN::gfx::gl::Vertex &vertex : meshData.vertices) {
        normalizeBoneWeights(vertex);
    }
}

int channelCountForFormat(SYN::gfx::gl::TextureFormat format) {
    switch (format) {
    case SYN::gfx::gl::TextureFormat::R8:
        return 1;
    case SYN::gfx::gl::TextureFormat::RG8:
        return 2;
    case SYN::gfx::gl::TextureFormat::RGB8:
    case SYN::gfx::gl::TextureFormat::SRGB:
        return 3;
    default:
        return 4;
    }
}

// Nearest-neighbour fetch of one channel, mapping a target-space texel back
// into the source. Lets us combine maps of differing resolutions.
uint8_t sampleChannelNearest(const SYN::gfx::gl::TextureData &tex, uint32_t tx,
                             uint32_t ty, uint32_t tw, uint32_t th,
                             int channel) {
    int channels = channelCountForFormat(tex.info.format);
    uint32_t sx = tw > 1 ? (tx * (tex.info.width - 1)) / (tw - 1) : 0;
    uint32_t sy = th > 1 ? (ty * (tex.info.height - 1)) / (th - 1) : 0;
    int ch = std::min(channel, channels - 1);
    return tex
        .data[(static_cast<size_t>(sy) * tex.info.width + sx) * channels + ch];
}

// Combine separate (or single) metalness/roughness maps into one packed
// texture in glTF layout (G = roughness, B = metallic). An absent channel is
// left at 1.0 so the scalar metallic/roughness factor drives it in the shader.
std::optional<SYN::gfx::gl::TextureData> packMetallicRoughness(
    const std::optional<SYN::gfx::gl::TextureData> &metalTex,
    const std::optional<SYN::gfx::gl::TextureData> &roughTex) {
    if (!metalTex && !roughTex) {
        return std::nullopt;
    }

    std::string cacheKey =
        "packedMR|" + (metalTex ? metalTex->sourcePath : std::string("none")) +
        "|" + (roughTex ? roughTex->sourcePath : std::string("none"));

    if (auto cached = g_TextureCache.find(cacheKey);
        cached != g_TextureCache.end()) {
        const CachedTexture &c = cached->second;
        return SYN::gfx::gl::TextureData{
            .info = {.width = c.width, .height = c.height, .format = c.format},
            .data = std::span<uint8_t>(c.storage->data(), c.storage->size()),
            .sourcePath = cacheKey};
    }

    uint32_t width = std::max(metalTex ? metalTex->info.width : 1u,
                              roughTex ? roughTex->info.width : 1u);
    uint32_t height = std::max(metalTex ? metalTex->info.height : 1u,
                               roughTex ? roughTex->info.height : 1u);

    auto storage = std::make_shared<std::vector<uint8_t>>();
    storage->resize(static_cast<size_t>(width) * height * 4);

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            uint8_t roughness =
                roughTex
                    ? sampleChannelNearest(*roughTex, x, y, width, height, 0)
                    : 255;
            uint8_t metallic = metalTex ? sampleChannelNearest(*metalTex, x, y,
                                                               width, height, 0)
                                        : 255;
            size_t dst = (static_cast<size_t>(y) * width + x) * 4;
            (*storage)[dst + 0] = 0;
            (*storage)[dst + 1] = roughness;
            (*storage)[dst + 2] = metallic;
            (*storage)[dst + 3] = 255;
        }
    }

    g_TextureCache[cacheKey] = CachedTexture{
        width, height, SYN::gfx::gl::TextureFormat::RGBA8, storage};

    return SYN::gfx::gl::TextureData{
        .info = {.width = width,
                 .height = height,
                 .format = SYN::gfx::gl::TextureFormat::RGBA8},
        .data = std::span<uint8_t>(storage->data(), storage->size()),
        .sourcePath = cacheKey};
}

// Decide whether a material needs alpha-cutout treatment (foliage, fences,
// grates) by inspecting its albedo and determining if the alpha channel meets
// the required threshold for alpha-cutout.
bool albedoNeedsAlphaMask(
    const std::optional<SYN::gfx::gl::TextureData> &albedo) {
    if (!albedo.has_value()) {
        return false;
    }

    const int channels = channelCountForFormat(albedo->info.format);
    // Only RGBA has an alpha channel
    if (channels < 4) {
        return false;
    }

    constexpr uint8_t kCutoffAlpha = 128;
    const SYN::gfx::gl::TextureData &tex = *albedo;
    const size_t pixelCount =
        static_cast<size_t>(tex.info.width) * tex.info.height;

    for (size_t i = 0; i < pixelCount; ++i) {
        if (tex.data[i * channels + (channels - 1)] < kCutoffAlpha) {
            return true;
        }
    }
    return false;
}

SYN::gfx::gl::MaterialData
processMaterial(const aiMaterial *material, const aiScene *scene,
                const std::filesystem::path &modelPath) {
    SYN::gfx::gl::MaterialData result{};

    aiColor4D color{};
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, color)) {
        result.tint = {color.r, color.g, color.b, color.a};
    }

    float metallic{};
    if (AI_SUCCESS == material->Get(AI_MATKEY_METALLIC_FACTOR, metallic)) {
        result.metallic = metallic;
    }

    float roughness{};
    if (AI_SUCCESS == material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness)) {
        result.roughness = roughness;
    }

    result.albedoData = loadTextureForMaterial(material, scene, modelPath,
                                               aiTextureType_BASE_COLOR);
    if (!result.albedoData.has_value()) {
        result.albedoData = loadTextureForMaterial(material, scene, modelPath,
                                                   aiTextureType_DIFFUSE);
    }

    result.alphaMasked = albedoNeedsAlphaMask(result.albedoData);

    result.normalData = loadTextureForMaterial(material, scene, modelPath,
                                               aiTextureType_NORMALS);
    if (!result.normalData.has_value()) {
        result.normalData = loadTextureForMaterial(material, scene, modelPath,
                                                   aiTextureType_HEIGHT);
    }

    // Metallic/roughness can be packed into one image (glTF: G=roughness,
    // B=metallic) or delivered as two separate grayscale maps (FBX/OBJ/USD).
    // Load both assimp slots and normalize to a single packed texture so the
    // renderer and shader only ever handle one metallicRoughnessMap.
    std::optional<SYN::gfx::gl::TextureData> metalData = loadTextureForMaterial(
        material, scene, modelPath, aiTextureType_METALNESS);
    std::optional<SYN::gfx::gl::TextureData> roughData = loadTextureForMaterial(
        material, scene, modelPath, aiTextureType_DIFFUSE_ROUGHNESS);

    bool sameImage = metalData && roughData &&
                     metalData->sourcePath == roughData->sourcePath;
    bool metalLooksPacked =
        metalData && channelCountForFormat(metalData->info.format) >= 3;
    bool roughLooksPacked =
        roughData && channelCountForFormat(roughData->info.format) >= 3;

    if (sameImage) {
        // Both slots point at one image: already packed (glTF).
        result.metallicRoughnessData = metalData;
    } else if (metalData && !roughData && metalLooksPacked) {
        // Only the metalness slot is set, but it's a multi-channel image — a
        // packed MR texture that some assimp versions expose only as METALNESS.
        result.metallicRoughnessData = metalData;
    } else if (roughData && !metalData && roughLooksPacked) {
        result.metallicRoughnessData = roughData;
    } else {
        // Separate grayscale maps (or just one present): combine into a packed
        // image. A missing channel stays 1.0 so its scalar factor drives it.
        result.metallicRoughnessData =
            packMetallicRoughness(metalData, roughData);
    }

    return result;
}

SYN::gfx::gl::MeshData
processMesh(const aiMesh *mesh, const aiScene *scene,
            const std::filesystem::path &modelPath,
            const glm::mat4 &localTransform,
            std::unordered_map<std::string, uint32_t> &boneIndexMap,
            uint32_t &nextBoneIndex) {
    SYN::gfx::gl::MeshData result{};
    result.localTransform = localTransform;

    // Local-space AABB accumulated over the raw vertex positions (before
    // localTransform). aabbToWorld() later applies cmd.transform *
    // mesh.localTransform, so the box must be in this pre-transform space.
    glm::vec3 aabbMin(std::numeric_limits<float>::max());
    glm::vec3 aabbMax(std::numeric_limits<float>::lowest());

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        SYN::gfx::gl::Vertex vertex{};
        vertex.position = {mesh->mVertices[i].x, mesh->mVertices[i].y,
                           mesh->mVertices[i].z};

        aabbMin = glm::min(aabbMin, vertex.position);
        aabbMax = glm::max(aabbMax, vertex.position);

        if (mesh->HasNormals()) {
            vertex.normal = {mesh->mNormals[i].x, mesh->mNormals[i].y,
                             mesh->mNormals[i].z};
        }

        if (mesh->HasTextureCoords(0)) {
            vertex.uv = {mesh->mTextureCoords[0][i].x,
                         mesh->mTextureCoords[0][i].y};
        }

        if (mesh->HasTangentsAndBitangents()) {
            glm::vec3 bitangent = {mesh->mBitangents[i].x,
                                   mesh->mBitangents[i].y,
                                   mesh->mBitangents[i].z};
            glm::vec3 tangent = {mesh->mTangents[i].x, mesh->mTangents[i].y,
                                 mesh->mTangents[i].z};
            glm::vec3 computedBitangent = glm::cross(vertex.normal, tangent);
            float handedness =
                glm::dot(bitangent, computedBitangent) < 0.0f ? -1.0f : 1.0f;
            vertex.tangent = glm::vec4(tangent, handedness);
        }

        result.vertices.emplace_back(vertex);
    }

    result.aabb = {aabbMin, aabbMax};

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        const aiFace &face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            result.indices.emplace_back(face.mIndices[j]);
        }
    }

    if (mesh->mMaterialIndex < scene->mNumMaterials) {
        result.material = processMaterial(
            scene->mMaterials[mesh->mMaterialIndex], scene, modelPath);
    }

    applyBonesToMesh(result, mesh, boneIndexMap, nextBoneIndex);
    return result;
}

void processNode(const aiNode *node, const aiScene *scene,
                 const std::filesystem::path &modelPath,
                 const glm::mat4 &parentTransform,
                 std::vector<SYN::gfx::gl::MeshData> &meshes,
                 std::unordered_map<std::string, uint32_t> &boneIndexMap,
                 uint32_t &nextBoneIndex) {
    glm::mat4 localTransform =
        parentTransform * toGlmMatrix(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.emplace_back(processMesh(mesh, scene, modelPath, localTransform,
                                        boneIndexMap, nextBoneIndex));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        processNode(node->mChildren[i], scene, modelPath, localTransform,
                    meshes, boneIndexMap, nextBoneIndex);
    }
}

} // namespace

std::optional<SYN::gfx::gl::ModelData>
SYN::gfx::gl::loadModelData(const std::filesystem::path &path) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(
        path.string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                           aiProcess_GenNormals | aiProcess_CalcTangentSpace |
                           aiProcess_FlipUVs | aiProcess_LimitBoneWeights);

    if (scene == nullptr || scene->mRootNode == nullptr) {
        spdlog::warn("Could not load {}: {}", path.string(),
                     importer.GetErrorString());
        return std::nullopt;
    }

    SYN::gfx::gl::ModelData modelData{};
    std::unordered_map<std::string, uint32_t> boneIndexMap{};
    uint32_t nextBoneIndex = 0;

    processNode(scene->mRootNode, scene, path, glm::mat4(1.0f),
                modelData.meshes, boneIndexMap, nextBoneIndex);

    return modelData;
}
