#pragma once

#include <filesystem>

#include "IAssetImporter.h"

namespace SYN {
template <typename AssetT> class AssetImporter : public IAssetImporter {
  public:
    virtual bool load(std::filesystem::path filepath, AssetT &asset,
                      class AssetManager *assetManager) {
        return false;
    };

    virtual bool loadGroup(std::filesystem::path filepath,
                           std::vector<std::pair<std::string, AssetT>> &assets,
                           class AssetManager *assetManager) {
        return false;
    };
};
} // namespace SYN
