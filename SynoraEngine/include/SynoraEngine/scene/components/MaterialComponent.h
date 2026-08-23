#pragma once

#include <SynoraEngine/project/AssetRef.h>

namespace SYN {
struct MaterialComponent {
    AssetRef material;
    uint32_t meshIndex = 0;
};
} // namespace SYN
