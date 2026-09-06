#pragma once

#include "../AssetRef.h"

#include <glm/vec4.hpp>

namespace SYN {

struct MaterialData {
    AssetRef albedoData;
    AssetRef normalData;
    AssetRef metallicRoughnessData;

    float metallic = 1.0f;
    float roughness = 1.0f;
    glm::vec4 tint = glm::vec4(1.0f);

    float alphaCutoff = 1.0f;

    std::string name;
};

} // namespace SYN
