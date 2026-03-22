#pragma once
#include <glm/glm.hpp>

struct alignas(16) Vertex {
    glm::vec3 pos;
    float u;
    alignas(16) float v;
};

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct TextureData {};

struct MaterialData {
    int temp;
};

using AssetType = std::variant<MeshData, MaterialData>;
template <typename T> struct is_variant : std::false_type {};

template <typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

template <typename T> inline constexpr bool is_variant_v = is_variant<T>::value;

template <typename T>
concept isAsset = requires() { is_variant_v<T>; };
