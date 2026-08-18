#pragma once

namespace SYN {

struct TextureData {
    uint32_t width;
    uint32_t height;
    uint32_t channelCount;

    std::vector<uint8_t> data;
    std::string sourcePath;
};
} // namespace SYN
