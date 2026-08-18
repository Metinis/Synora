#pragma once

#include "../AssetImporter.h"
#include "../assets/TextureData.h"

namespace SYN {
class TextureImporter : public AssetImporter<TextureData> {
  public:
    bool load(std::filesystem::path filepath, TextureData &asset,
              class AssetManager *assetManager) override;
};
} // namespace SYN
