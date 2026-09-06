#pragma once

namespace SYN {

struct TextureData {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channelCount = 0;

    std::vector<uint8_t> data{};

    // Only filled if it's hdr
    std::vector<float> dataFloat{};

    std::string sourcePath;
};
} // namespace SYN
