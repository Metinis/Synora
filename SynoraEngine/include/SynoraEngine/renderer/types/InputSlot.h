#pragma once

#include <glm/glm.hpp>

#include <SynoraEngine/project/AssetRef.h>

#include "SamplerDesc.h"

namespace SYN {
struct InputSlot {
    struct RenderTargetInput {
        std::variant<std::string, AssetRef> target;

        enum class AttachmentType {
            Color0,
            Color1,
            Color2,
            Color3,
            Color4,
            Color5,
            Color6,
            Color7,

            DepthStencil
        };

        uint32_t bindingIndex;
        AttachmentType attachment;
        SamplerDesc sampler;
    };

    // TODO: Add texture cubemaps as possible input type.
    struct Texture2DInput {
        AssetRef texture;
        uint32_t bindingIndex;
        SamplerDesc sampler;
        bool srgb = false;
    };

    using Value = std::variant<
        float, glm::vec2, glm::vec3, glm::vec4, int32_t, glm::ivec2, glm::ivec3,
        glm::ivec4, uint32_t, glm::uvec2, glm::uvec3, glm::uvec4, glm::mat4,
        std::span<const glm::mat4>, RenderTargetInput, Texture2DInput>;

    std::string inputName;
    Value value;
};
} // namespace SYN
