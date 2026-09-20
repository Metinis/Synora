#pragma once

#include <glm/glm.hpp>

#include <SynoraEngine/core/math/AABB.h>
#include <SynoraEngine/project/UUID.h>
#include <SynoraEngine/renderer/types/InputSlot.h>

namespace SYN {

struct MaterialView {
    uint32_t modelIndex;
    uint32_t meshIndex;
    UUID material;
};

struct AnimationView {
    uint32_t modelIndex;
    std::span<const glm::mat4> boneMatrices;
};

struct BoundsView {
    std::vector<AABB> meshBounds;
};

struct CustomRenderDataView {
    using Table = std::unordered_map<std::string, std::span<const InputSlot>>;
    uint32_t modelIndex;
    Table effectInputs;
};

struct RenderView3D {
    std::vector<UUID> models;
    std::vector<uint64_t> layers;
    std::vector<BoundsView> bounds;
    std::vector<glm::mat4> transforms;
    std::vector<MaterialView> materials;
    std::vector<AnimationView> animations;
    std::vector<CustomRenderDataView> customRenderData;

    static RenderView3D fromScene(class Scene *scene);
};
}; // namespace SYN
