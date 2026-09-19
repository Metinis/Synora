#pragma once

#include "PipelineState.h"
#include <glm/vec4.hpp>

namespace SYN {
struct SamplerDesc {
    enum class Filter {
        Nearest,
        Linear,
        NearestMipmapNearest,
        LinearMipmapLinear,
    };

    enum class WrapMode { ClampToEdge, Repeat, MirroredRepeat, ClampToBorder };

    Filter minFilter = Filter::Linear;
    Filter magFilter = Filter::Linear;

    WrapMode wrapU = WrapMode::Repeat;
    WrapMode wrapV = WrapMode::Repeat;
    WrapMode wrapW = WrapMode::Repeat;

    glm::vec4 borderColor = glm::vec4(1.0f);

    std::optional<PipelineState::CompareFunc> compareFunc = std::nullopt;

    float anisotropy = 1.0f;
};
} // namespace SYN
