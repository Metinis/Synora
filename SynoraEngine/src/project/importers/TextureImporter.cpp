#include <SynoraEngine/project/importers/TextureImporter.h>

#include <spdlog/spdlog.h>
#include <stb_image.h>

namespace SYN {
bool TextureImporter::load(std::filesystem::path filepath, TextureData &asset,
                           class AssetManager *assetManager) {
    int32_t width, height, channelCount;
    stbi_uc *data =
        stbi_load(filepath.c_str(), &width, &height, &channelCount, 0);
    if (data == nullptr) {
        spdlog::error("{}", stbi_failure_reason());
        return false;
    }

    asset.width = width;
    asset.height = height;
    asset.channelCount = channelCount;
    asset.sourcePath = filepath.string();

    asset.data.assign(data, data + (width * height * channelCount));

    stbi_image_free(data);

    return true;
}
} // namespace SYN
