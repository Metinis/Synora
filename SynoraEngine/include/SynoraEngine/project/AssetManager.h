#pragma once

#include "Assets.h"
#include "UUID.h"
#include "stb_image.h"

namespace SYN {
class Entity;
struct AssetCounted {
    AssetType data{};
    uint64_t ref{};
};
class AssetManager {
  public:
    AssetManager() = default;
    ~AssetManager() = default;

  private:
    std::unordered_map<UUID, AssetCounted> m_AssetMap{};
    std::unordered_map<std::string, UUID> m_LoadedUUIDMap{};
};
} // namespace SYN
