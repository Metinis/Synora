#pragma once
#include "Assets.h"
#include "PuzzleEngine/core/InputTypes.h"
#include "PuzzleEngine/scene/Components.h"
#include "UUID.h"
#include "renderer/Renderer.h"
#include "spdlog/spdlog.h"
#include "stb_image.h"

#include <assimp/Importer.hpp>

struct aiNode;
struct aiMesh;
struct aiScene;

namespace SYN {

class AssetManager {
  public:
    AssetManager() = default;

    void init(EngineContext *ctx);

    UUID loadModel(const std::string &path);
    UUID loadTexture(const std::string &path);

    // automatically gen ID
    template <typename T>
        requires isAsset<T>
    UUID addAsset(T asset) {
        UUID id = generateUUID();
        m_AssetMap[id] = asset;
        // generate mesh data if we added one
        spdlog::debug("Asset added: {}", id);
        if constexpr (std::is_same_v<T, ModelData>) {
            m_Renderer->addModel(id, asset);
        }
        return id;
    }

    // to be used when we manually gen ID
    template <typename T>
        requires isAsset<T>
    void createAsset(T asset, UUID id) {
        m_AssetMap[id] = asset;
        // generate model data if we added one
        spdlog::debug("Asset created: %d", id);
        if (std::holds_alternative<ModelData>(asset)) {
            m_Renderer->addModel(id, std::get<ModelData>(asset));
        }
    }

    template <typename T>
        requires isAsset<T>
    T *get(UUID id) {
        // check if the id is what the user requested
        auto it{m_AssetMap.find(id)};
        if (it != m_AssetMap.end() &&
            std::holds_alternative<T>(m_AssetMap[id])) {
            return &std::get<T>(it->second);
        }
        spdlog::error("Asset not found, ID!: %d Type: %s", id,
                      typeid(T).name());
        return nullptr;
    }

    ~AssetManager() = default;

  private:
    // NOTE: textures are malloced and must be freed. currently they leak.

    // no file format or compression, just bytes and sizes
    UUID loadRawTexture(stbi_uc *data, uint32_t width, uint32_t height,
                        const std::string &name);
    UUID loadTexture(stbi_uc *data, uint32_t size,
                     const std::string &modelPath);

    MeshData processMesh(const aiMesh *mesh, const aiScene *scene,
                         const std::string &modelPath,
                         const aiMatrix4x4 &transform);

    void processNode(aiNode *node, const aiScene *scene,
                     std::vector<SYN::MeshData> &meshes,
                     const std::string &modelPath,
                     const aiMatrix4x4 &parentTransform);

    // currently only processes albedo texture
    SYN::UUID processMaterials(const aiMesh *mesh, const aiScene *scene,
                               const std::string &path);

    std::unordered_map<UUID, AssetType> m_AssetMap{};
    std::unordered_map<std::string, UUID> m_LoadedUUIDMap{};
    Renderer *m_Renderer{};
    Assimp::Importer m_Importer;
    UUID m_MissingTextureUUID{};
};
} // namespace SYN
