#pragma once

#include "SynoraEngine/renderer/Renderer.h"
#include "Assets.h"
#include "SynoraEngine/core/InputTypes.h"
#include "SynoraEngine/scene/Components.h"
#include "UUID.h"
#include "spdlog/spdlog.h"
#include "stb_image.h"

#include "assimp/material.h"
#include <assimp/Importer.hpp>

struct aiNode;
struct aiMesh;
struct aiScene;

namespace SYN {
class Entity;
struct AssetCounted {
    AssetType data{};
    uint64_t ref{};
};
class AssetManager {
  public:
    AssetManager() = default;

    void init(EngineContext *ctx);

    void loadModel(Scene *scene, const std::filesystem::path &path);
    UUID loadTexture(const std::string &path);

    void addRef(UUID id);
    void removeRef(UUID id);

    // automatically gen ID
    template <typename T>
        requires isAsset<T>
    UUID addAsset(T asset) {
        UUID id = generateUUID();
        m_AssetMap[id].data = asset;
        // generate mesh data if we added one
        spdlog::debug("Asset added: {}", id);
        return id;
    }

    // to be used when we manually gen ID
    template <typename T>
        requires isAsset<T>
    void createAsset(T asset, UUID id) {
        m_AssetMap[id].data = asset;
        // generate model data if we added one
        spdlog::debug("Asset created: %d", id);
    }

    template <typename T>
        requires isAsset<T>
    T *get(UUID id) {
        // check if the id is what the user requested
        auto it{m_AssetMap.find(id)};
        if (it != m_AssetMap.end() &&
            std::holds_alternative<T>(m_AssetMap[id].data)) {
            return &std::get<T>(it->second.data);
        }
        spdlog::error("Asset not found, ID!: %d Type: %s", id,
                      typeid(T).name());
        return nullptr;
    }

    ~AssetManager() = default;

  private:
    struct Material {
        UUID albedo;
        UUID metallicRoughness;
        UUID normalMap;
    };

    // NOTE: textures are malloced and must be freed. currently they leak.

    // no file format or compression, just bytes and sizes
    UUID loadRawTexture(stbi_uc *data, uint32_t width, uint32_t height,
                        const std::string &name);
    UUID loadTexture(stbi_uc *data, uint32_t size,
                     const std::string &modelPath);

    MeshData processMesh(const aiMesh *mesh, const aiScene *scene,
                         const std::string &modelPath,
                         const aiMatrix4x4 &transform);

    void processNode(Scene *scene, Entity parent, aiNode *node,
                     const aiScene *aiScene, const std::string &modelPath,
                     const aiMatrix4x4 &parentTransform);

    std::optional<SYN::UUID> loadMaterial(const aiMesh *mesh,
                                          const aiScene *scene,
                                          const std::string &modelPath,
                                          aiTextureType textureType);

    Material processMaterials(const aiMesh *mesh, const aiScene *scene,
                              const std::string &path);

    std::unordered_map<UUID, AssetCounted> m_AssetMap{};
    std::unordered_map<std::string, UUID> m_LoadedUUIDMap{};
    Renderer *m_Renderer{};
    Assimp::Importer m_Importer;

    UUID m_MissingTextureUUID{};
    UUID m_DefaultMetallicRoughness{};
    UUID m_WhiteTexture{};
    UUID m_DefaultNormalMap{};
};
} // namespace SYN
