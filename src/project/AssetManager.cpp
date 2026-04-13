#include "PuzzleEngine/project/AssetManager.h"
#include "assimp/cimport.h"
#include "assimp/material.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>

void SYN::AssetManager::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    stbi_set_flip_vertically_on_load(true);
    m_MissingTextureUUID =
        loadTexture("resources/textures/missing_texture.png");
}

SYN::UUID SYN::AssetManager::loadTexture(const std::string &path) {
    if (m_LoadedUUIDMap.contains(path)) {
        return m_LoadedUUIDMap[path];
    }
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
        return m_MissingTextureUUID;
    }

    TextureData textureData{
        .width = static_cast<uint32_t>(imageWidth),
        .height = static_cast<uint32_t>(imageHeight),
        .data = imageBytes,
    };

    UUID uuid{generateUUID()};
    m_AssetMap[uuid].data = textureData;
    m_LoadedUUIDMap[path] = uuid;

    return uuid;
}

void SYN::AssetManager::addRef(UUID id) {
    if (m_AssetMap.contains(id)) {
        spdlog::debug("Asset Manager: {} Ref increased", id);
        if (m_AssetMap[id].ref == 0) {
            //add to renderer if an entity uses it
            auto asset = m_AssetMap[id].data;
            if (std::holds_alternative<ModelData>(asset)) {
                m_Renderer->addModel(id, std::get<ModelData>(asset));
            }
        }
        m_AssetMap[id].ref++;
    } else {
        spdlog::warn("Asset Manager: Could not add ref {}, asset missing", id);
    }
}

void SYN::AssetManager::removeRef(UUID id) {
    if (m_AssetMap.contains(id)) {
        spdlog::debug("Asset Manager: {} Ref decreased", id);
        m_AssetMap[id].ref--;
        if (m_AssetMap[id].ref == 0) {
            //remove from renderer
            auto asset = m_AssetMap[id].data;
            if (std::holds_alternative<ModelData>(asset)) {
                m_Renderer->removeModel(id);
            }
        }
    } else {
        spdlog::warn("Asset Manager: Could not remove ref {}, asset missing", id);
    }
}

SYN::UUID SYN::AssetManager::loadRawTexture(stbi_uc *data, uint32_t width,
                                            uint32_t height,
                                            const std::string &name) {
    if (m_LoadedUUIDMap.contains(name)) {
        spdlog::debug("{} was already loaded, cache hit", name);
        return m_LoadedUUIDMap[name];
    }
    spdlog::debug("Loading {}", name);

    TextureData textureData{
        .width = width,
        .height = height,
        .data = data,
    };

    UUID uuid{generateUUID()};
    m_AssetMap[uuid].data = textureData;
    m_LoadedUUIDMap[name] = uuid;

    return uuid;
}
SYN::UUID SYN::AssetManager::loadTexture(stbi_uc *data, uint32_t size,
                                         const std::string &name) {
    if (m_LoadedUUIDMap.contains(name)) {
        return m_LoadedUUIDMap[name];
    }
    spdlog::debug("Loading {}", name);

    int imageWidth{};
    int imageHeight{};
    int channelsInImage{};
    stbi_uc *imageBytes{stbi_load_from_memory(data, static_cast<int>(size),
                                              &imageWidth, &imageHeight,
                                              &channelsInImage, 4)};

    assert(imageWidth >= 0);
    assert(imageHeight >= 0);
    if (imageBytes == nullptr) {
        spdlog::warn("Could not load {}", name);
        return m_MissingTextureUUID;
    }

    TextureData textureData{
        .width = static_cast<uint32_t>(imageWidth),
        .height = static_cast<uint32_t>(imageHeight),
        .data = imageBytes,
    };

    UUID uuid{generateUUID()};
    m_AssetMap[uuid].data = textureData;
    m_LoadedUUIDMap[name] = uuid;

    return uuid;
}

SYN::UUID SYN::AssetManager::loadModel(const std::string &path) {
    if (m_LoadedUUIDMap.contains(path)) {
        return m_LoadedUUIDMap[path];
    }
    spdlog::debug("Loading {}", path);

    const aiScene *scene{m_Importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                  aiProcess_SortByPType)};

    if (scene == nullptr) {
        spdlog::warn("Could not load {}", aiGetErrorString());
        return 0; // TODO: make a default model
    }

    std::vector<MeshData> meshes{};
    aiMatrix4x4 origin{};
    aiIdentityMatrix4(&origin);
    processNode(scene->mRootNode, scene, meshes, path, origin);

    SYN::ModelData modelData{.meshes = std::move(meshes)};

    UUID uuid{generateUUID()};
    m_AssetMap[uuid].data = modelData;
    m_LoadedUUIDMap[path] = uuid;

    return uuid;
}

SYN::UUID SYN::AssetManager::processMaterials(const aiMesh *mesh,
                                              const aiScene *scene,
                                              const std::string &modelPath) {
    if (!scene->HasMaterials()) {
        spdlog::warn("{} had no materials", modelPath);
        return m_MissingTextureUUID;
    }

    aiMaterial *material{scene->mMaterials[mesh->mMaterialIndex]};
    aiString relPath{};

    if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &relPath) !=
        AI_SUCCESS) {
        // fallback to diffuse
        if (material->GetTexture(aiTextureType_DIFFUSE, 0, &relPath) !=
            AI_SUCCESS) {
            return m_MissingTextureUUID;
        }
    }
    bool isEmbeddedTexture{relPath.C_Str()[0] == '*'};

    std::filesystem::path absDir{
        std::filesystem::path(modelPath).parent_path()};
    std::string absPath{absDir.append(relPath.C_Str())};

    if (!isEmbeddedTexture) {
        return loadTexture(absPath);
    }

    const aiTexture *texture{scene->GetEmbeddedTexture(relPath.C_Str())};
    std::string name{absPath};

    bool isCompressed{texture->mHeight == 0};
    if (isCompressed) {
        uint32_t textureSize{texture->mWidth};

        return loadTexture(reinterpret_cast<stbi_uc *>(texture->pcData),
                           textureSize, name);
    }

    // textures are stored ARGB, when we store them as RGBA, so we need to
    // swizzle
    size_t pixelCount{texture->mWidth * texture->mHeight};
    auto *pixels{reinterpret_cast<aiTexel *>(texture->pcData)};

    auto *rgbaData{reinterpret_cast<stbi_uc *>(malloc(pixelCount * 4))};
    if (rgbaData == nullptr) {
        spdlog::warn("Could not allocate memory for {} albedo", modelPath);
        return m_MissingTextureUUID;
    }

    for (size_t i{}; i < pixelCount; i++) {
        rgbaData[i * 4 + 0] = pixels[i].r;
        rgbaData[i * 4 + 1] = pixels[i].g;
        rgbaData[i * 4 + 2] = pixels[i].b;
        rgbaData[i * 4 + 3] = pixels[i].a;
    }

    return loadRawTexture(rgbaData, texture->mWidth, texture->mHeight, name);
}

SYN::MeshData SYN::AssetManager::processMesh(const aiMesh *mesh,
                                             const aiScene *scene,
                                             const std::string &modelPath,
                                             const aiMatrix4x4 &transform) {
    SYN::MeshData processedMesh{
        .localTransform =
            glm::mat4(transform.a1, transform.b1, transform.c1, transform.d1,
                      transform.a2, transform.b2, transform.c2, transform.d2,
                      transform.a3, transform.b3, transform.c3, transform.d3,
                      transform.a4, transform.b4, transform.c4, transform.d4),
    };

    UUID albedoUUID{processMaterials(mesh, scene, modelPath)};
    processedMesh.albedo = get<TextureData>(albedoUUID);

    for (size_t i{}; i < mesh->mNumVertices; i++) {
        SYN::Vertex relativeVertex{
            .pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                             mesh->mVertices[i].z),
        };

        if (mesh->mTextureCoords[0]) {
            relativeVertex.u = mesh->mTextureCoords[0][i].x;
            relativeVertex.v = mesh->mTextureCoords[0][i].y;
        }

        processedMesh.vertices.emplace_back(relativeVertex);
    }

    for (size_t i{}; i < mesh->mNumFaces; i++) {
        aiFace face{mesh->mFaces[i]};
        for (size_t j{}; j < face.mNumIndices; j++) {
            processedMesh.indices.emplace_back(face.mIndices[j]);
        }
    }

    return processedMesh;
}
void SYN::AssetManager::processNode(aiNode *node, const aiScene *scene,
                                    std::vector<SYN::MeshData> &meshes,
                                    const std::string &modelPath,
                                    const aiMatrix4x4 &parentTransform) {
    aiMatrix4x4 transform = parentTransform * node->mTransformation;

    for (size_t i{}; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.emplace_back(processMesh(mesh, scene, modelPath, transform));
    }

    for (size_t i{}; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, meshes, modelPath, transform);
    }
}
