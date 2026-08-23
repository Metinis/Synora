#pragma once

#include <SynoraEngine/project/AssetRef.h>

namespace SYN {
struct MaterialComponent {
    struct Submesh {
        AssetRef material;
        uint32_t meshIndex = 0;
    };
    std::vector<Submesh> submeshes;
};
} // namespace SYN
