#pragma once

namespace SYN {

struct OpenGLConfig {
    uint32_t versionMajor = 4;
    uint32_t versionMinor = 5;
};

struct WindowConfig {
    std::string_view title;
    uint16_t width;
    uint16_t height;

    std::optional<OpenGLConfig> openGLConfig = std::nullopt;
};

} // namespace SYN
