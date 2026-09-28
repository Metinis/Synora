#pragma once

#include <SynoraEngine/project/AssetRef.h>

#include "RenderTargetData.h"

namespace SYN {
struct TextureViewData {
    RenderTargetData::Format format;
    AssetRef target;

    enum class DepthKind { DepthStencil, DepthOnly, StencilOnly };
    // Only used if target format is DepthStencil
    DepthKind depthKind = DepthKind::DepthStencil;

    uint32_t colorAttachment = 0;
    uint32_t minLevel = 0, numLevels = 1;
    uint32_t minLayer = 0, numLayers = 1;
};
} // namespace SYN
