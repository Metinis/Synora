#pragma once

#include <glm/vec4.hpp>

namespace SYN {
struct ClearOptions {
    std::optional<glm::vec4> clearColor = std::nullopt;
    std::optional<float> clearDepth = std::nullopt;
    std::optional<uint8_t> clearStencil = std::nullopt;
};
} // namespace SYN
