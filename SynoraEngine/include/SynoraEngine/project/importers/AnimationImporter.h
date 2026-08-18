#pragma once

#include "../AssetImporter.h"
#include "../assets/AnimationClipData.h"

namespace SYN {
class AnimationImporter : public AssetImporter<AnimationClipData> {
  public:
    bool load(std::filesystem::path filepath, AnimationClipData &asset,
              class AssetManager *assetManager) override;
};
} // namespace SYN
