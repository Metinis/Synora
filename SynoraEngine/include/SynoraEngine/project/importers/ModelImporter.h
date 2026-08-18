#pragma once

#include "../AssetImporter.h"

#include "../assets/ModelData.h"

namespace SYN {

class ModelImporter : public AssetImporter<ModelData> {
  public:
    bool load(std::filesystem::path filepath, ModelData &asset,
              class AssetManager *assetManager) override;

  private:
    void processNode(const class aiNode *current, int32_t parentIndex,
                     const glm::mat4 &parentTransform, Skeleton &skeleton);

    MeshData processMesh(const class aiMesh *mesh,
                         const glm::mat4 &localTransform,
                         const std::filesystem::path &path, Skeleton &skeleton);

    AssetRef processMaterial(const class aiMaterial *material,
                             const std::filesystem::path &path);

    AssetRef loadTextureForMaterial(std::filesystem::path path,
                                    const class aiMaterial *material,
                                    uint32_t type);

    void applyBonesToMesh(MeshData &meshData, const class aiMesh *mesh,
                          Skeleton &skeleton);

  private:
    std::unordered_map<std::string, uint32_t> m_BoneIndexMap{};
    std::unordered_map<std::string, uint32_t> m_NodeNameToIndex{};
    std::unordered_map<std::string, glm::mat4> m_MeshGlobalTransform{};

    uint32_t m_NextBoneIndex = 0;
    uint32_t m_DefaultMaterialIndex = 0;

    const class aiScene *m_Scene;
    class AssetManager *m_AssetManager;
};
} // namespace SYN
