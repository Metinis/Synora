#pragma once

#include <filesystem>

#include "IAssetImporter.h"

namespace SYN {
template <typename AssetT> class AssetImporter : public IAssetImporter {
  public:
    virtual bool load(std::filesystem::path filepath, AssetT &asset,
                      class AssetManager *assetManager) = 0;
};
} // namespace SYN
