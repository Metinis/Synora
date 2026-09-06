#include <SynoraEngine/project/importers/TextureImporter.h>

#include <spdlog/spdlog.h>
#include <stb_image.h>

namespace SYN {
bool TextureImporter::load(std::filesystem::path filepath, TextureData &asset,
                           class AssetManager *assetManager) {
    int32_t width, height, channelCount;

    if (filepath.extension() == ".hdr") {
        // HDR is forced to 4 channels per pixel because I can't imagine a case
        // where the texture should be < 4, and also because creating the
        // prefiltered/irradiance map doesn't work properly if we stick to 3
        // channels on our source texture.
        float *data = stbi_loadf(filepath.string().c_str(), &width, &height,
                                 &channelCount, 4);

        if (data == nullptr) {
            spdlog::error("{}", stbi_failure_reason());
            return false;
        }

        channelCount = 4;
        asset.dataFloat.assign(data, data + (width * height * 4));
        stbi_image_free(data);
    } else {
        stbi_uc *data = stbi_load(filepath.string().c_str(), &width, &height,
                                  &channelCount, 0);

        if (data == nullptr) {
            spdlog::error("{}", stbi_failure_reason());
            return false;
        }

        asset.data.assign(data, data + (width * height * channelCount));
        stbi_image_free(data);
    }

    asset.width = width;
    asset.height = height;
    asset.channelCount = channelCount;
    asset.sourcePath = filepath.string();

    return true;
}
} // namespace SYN
