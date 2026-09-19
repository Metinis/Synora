#pragma once

#include <SynoraEngine/project/AssetRef.h>

namespace SYN {
struct ModelComponent {
    AssetRef model;
    uint64_t layer = 1;
};
} // namespace SYN
