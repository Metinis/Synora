#include "SynoraEngine/project/AssetManager.h"
#include "SynoraEngine/scene/Entity.h"
#include "SynoraEngine/scene/Scene.h"
#include "assimp/cimport.h"
#include "assimp/material.h"
#include "assimp/mesh.h"
#include "assimp/postprocess.h"
#include "glm/gtx/matrix_decompose.hpp"
#include <assimp/postprocess.h>
#include <assimp/scene.h>


void SYN::AssetManager::init(EngineContext *ctx) {
    m_Renderer = ctx->renderer.get();
    stbi_set_flip_vertically_on_load(true);
    m_MissingTextureUUID =
        loadTexture("resources/textures/missing_texture.png");
    m_DefaultMetallicRoughness =
        loadTexture("resources/textures/default_metallic_roughness.png");
    m_WhiteTexture = loadTexture("resources/textures/white.png");
    m_DefaultNormalMap =
        loadTexture("resources/textures/default_normal_map.png");
}

SYN::UUID SYN::AssetManager::loadTexture(const std::string &path) {
    if (m_LoadedUUIDMap.contains(path)) {
        return m_LoadedUUIDMap[path];
    }
    SYN_LOG_ASSET("Loading {}", path);

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
        SYN_LOG_ASSET("Asset Manager: {} Ref increased", id);
        if (m_AssetMap[id].ref == 0) {
            // add to renderer if an entity uses it
            auto asset = m_AssetMap[id].data;
            if (std::holds_alternative<MeshData>(asset)) {
                m_Renderer->addMesh(id, std::get<MeshData>(asset));
            }
        }
        m_AssetMap[id].ref++;
    } else if (id != 0){
        spdlog::warn("Asset Manager: Could not add ref {}, asset missing", id);
    }
}

void SYN::AssetManager::removeRef(UUID id) {
    if (m_AssetMap.contains(id)) {
        SYN_LOG_ASSET("Asset Manager: {} Ref decreased", id);
        m_AssetMap[id].ref--;
        if (m_AssetMap[id].ref == 0) {
            // remove from renderer
            auto asset = m_AssetMap[id].data;
            if (std::holds_alternative<MeshData>(asset)) {
                m_Renderer->removeMesh(id);
            }
        }
    } else if (id != 0){
        spdlog::warn("Asset Manager: Could not remove ref {}, asset missing",
                     id);
    }
}

SYN::UUID SYN::AssetManager::loadRawTexture(stbi_uc *data, uint32_t width,
                                            uint32_t height,
                                            const std::string &name) {
    if (m_LoadedUUIDMap.contains(name)) {
        SYN_LOG_ASSET("{} was already loaded, cache hit", name);
        return m_LoadedUUIDMap[name];
    }
    SYN_LOG_ASSET("Loading {}", name);

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
    SYN_LOG_ASSET("Loading {}", name);

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

void SYN::AssetManager::loadModel(Scene *scene,
                                  const std::filesystem::path &path) {
    SYN_LOG_ASSET("Loading {}", path.c_str());
    const aiScene *aiScene{m_Importer.ReadFile(
        path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                  aiProcess_SortByPType)};

    if (scene == nullptr) {
        spdlog::warn("Could not load {}", aiGetErrorString());
        return; // TODO: make a default model
    }

    std::vector<MeshData> meshes{};
    Entity parent = scene->createEntity();
    // std::string entName = path.substr(0, path.find_last_of('/'));
    parent.getComponent<TagComp>() = TagComp{.tag = path.stem().string()};
    processNode(scene, parent, aiScene->mRootNode, aiScene, path);
}

SYN::AssetManager::Material
SYN::AssetManager::processMaterials(const aiMesh *mesh, const aiScene *scene,
                                    const std::string &modelPath) {
    Material material{
        .albedo = m_MissingTextureUUID,
        .metallicRoughness = m_MissingTextureUUID,
    };

    if (!scene->HasMaterials()) {
        spdlog::warn("{} had no materials", modelPath);
        return material;
    }

    std::optional<UUID> albedo{
        loadMaterial(mesh, scene, modelPath, aiTextureType_BASE_COLOR)};

    std::optional<UUID> metallicRoughness{loadMaterial(
        mesh, scene, modelPath, aiTextureType_GLTF_METALLIC_ROUGHNESS)};

    std::optional<UUID> normalMap{
        loadMaterial(mesh, scene, modelPath, aiTextureType_NORMALS)};

    if (!albedo.has_value()) {
        albedo = loadMaterial(mesh, scene, modelPath, aiTextureType_DIFFUSE);
        if (!albedo.has_value()) {
            albedo = m_MissingTextureUUID;
        }
    }

    if (!metallicRoughness.has_value()) {
        metallicRoughness = m_DefaultMetallicRoughness;
    }

    if (!normalMap.has_value()) {
        normalMap = m_DefaultNormalMap;
    }

    material.albedo = albedo.value();
    material.metallicRoughness = metallicRoughness.value();
    material.normalMap = normalMap.value();

    return material;
}

SYN::MeshData SYN::AssetManager::processMesh(const aiMesh *mesh,
                                             const aiScene *scene,
                                             const std::string &modelPath) {
    SYN::MeshData processedMesh{};

    Material material{processMaterials(mesh, scene, modelPath)};

    processedMesh.albedo = get<TextureData>(material.albedo);
    processedMesh.metallicRoughness =
        get<TextureData>(material.metallicRoughness);
    processedMesh.normalMap = get<TextureData>(material.normalMap);

    for (size_t i{}; i < mesh->mNumVertices; i++) {
        SYN::Vertex relativeVertex{
            .pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y,
                             mesh->mVertices[i].z),
        };

        if (mesh->mTextureCoords[0]) {
            relativeVertex.u = mesh->mTextureCoords[0][i].x;
            relativeVertex.v = mesh->mTextureCoords[0][i].y;
        }
        glm::vec3 normal(0.f, 0.f, -1.f);
        if (mesh->HasNormals()) {
            normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y,
                               mesh->mNormals[i].z);
        }

        glm::vec4 tangentAndHandedness(0.f, 1.f, 0.f, 1.f);
        if (mesh->HasTangentsAndBitangents()) {
            glm::vec3 tangent{glm::vec3(mesh->mTangents[i].x,
                                        mesh->mTangents[i].y,
                                        mesh->mTangents[i].z)};
            glm::vec3 bitangent{glm::vec3(mesh->mBitangents[i].x,
                                          mesh->mBitangents[i].y,
                                          mesh->mBitangents[i].z)};
            float handedness{glm::dot(glm::cross(normal, tangent), bitangent)};
            tangentAndHandedness =
                glm::vec4(tangent.x, tangent.y, tangent.z, handedness);
        }

        relativeVertex.normal = normal;
        relativeVertex.tangent = tangentAndHandedness;

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
void SYN::AssetManager::processNode(Scene *scene, Entity parent, aiNode *node,
                                    const aiScene *aiScene,
                                    const std::string &modelPath) {
    aiMatrix4x4 transform = node->mTransformation;

    auto transformm = glm::mat4(transform.a1, transform.b1, transform.c1, transform.d1,
                      transform.a2, transform.b2, transform.c2, transform.d2,
                      transform.a3, transform.b3, transform.c3, transform.d3,
                      transform.a4, transform.b4, transform.c4, transform.d4);

    glm::vec3 scale;
    glm::quat rotation;
    glm::vec3 translation;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(transformm, scale, rotation, translation, skew, perspective);

    auto childEntity = scene->createEntity(node->mName.C_Str());
    childEntity.getComponent<TransformComp>().position = translation;
    childEntity.getComponent<TransformComp>().rotation = rotation;
    childEntity.getComponent<TransformComp>().scale = scale;
    childEntity.addComponent<ParentComp>(
        ParentComp{.id = parent.getComponent<UUIDComp>().id});

    for (size_t i{}; i < node->mNumMeshes; i++) {
        aiMesh *mesh = aiScene->mMeshes[node->mMeshes[i]];
        UUID uuid{generateUUID()};
        m_AssetMap[uuid].data =
            processMesh(mesh, aiScene, modelPath);
        m_LoadedUUIDMap[modelPath] = uuid;
        Entity meshEnt = scene->createEntity(mesh->mName.C_Str());
        meshEnt.addComponent<MeshComp>(MeshComp{.id = uuid});
        meshEnt.addComponent<ParentComp>(
            ParentComp{.id = childEntity.getComponent<UUIDComp>().id});
    }

    for (size_t i{}; i < node->mNumChildren; i++) {
        processNode(scene, childEntity, node->mChildren[i], aiScene, modelPath);
    }
}

std::optional<SYN::UUID>
SYN::AssetManager::loadMaterial(const aiMesh *mesh, const aiScene *scene,
                                const std::string &modelPath,
                                aiTextureType textureType) {
    aiMaterial *material{scene->mMaterials[mesh->mMaterialIndex]};
    aiString relPath{};

    if (material->GetTexture(textureType, 0, &relPath) != AI_SUCCESS) {
        return {};
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
        return {};
    }

    for (size_t i{}; i < pixelCount; i++) {
        rgbaData[i * 4 + 0] = pixels[i].r;
        rgbaData[i * 4 + 1] = pixels[i].g;
        rgbaData[i * 4 + 2] = pixels[i].b;
        rgbaData[i * 4 + 3] = pixels[i].a;
    }

    return loadRawTexture(rgbaData, texture->mWidth, texture->mHeight, name);
}
