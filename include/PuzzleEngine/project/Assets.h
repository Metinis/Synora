#pragma once
#include <PuzzleEngine/project/UUID.h>
#include <glm/glm.hpp>
#include <stb_image.h>

namespace SYN {

struct alignas(16) Vertex {
    glm::vec3 pos;
    float u;
    glm::vec3 normal;
    float v;
};

struct TextureData {
    uint32_t width;
    uint32_t height;
    stbi_uc *data; // this is malloced, make sure to free, though im not doing
                   // that yet cus this is test code
};

struct MeshData {
    std::vector<Vertex> vertices; // these are relative to some parent
    std::vector<uint32_t> indices;
    glm::mat4 localTransform; // brings vertices from relative to parent to
                              // relative to world space

    TextureData *albedo;
};

struct ModelData {
    std::vector<MeshData> meshes;
};

struct MaterialData {
    int temp;
};

using AssetType = std::variant<MeshData, MaterialData, TextureData, ModelData>;
template <typename T> struct is_variant : std::false_type {};

template <typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

template <typename T> inline constexpr bool is_variant_v = is_variant<T>::value;

template <typename T>
concept isAsset = requires() { is_variant_v<T>; };
} // namespace SYN
