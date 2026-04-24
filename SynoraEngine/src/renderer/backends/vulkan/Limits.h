#pragma once

#include "CreateInfos.h"

namespace SYN::VK {

namespace Limits {
constexpr uint32_t c_TextureBinding{0};
constexpr uint32_t c_SamplerBinding{1};
constexpr uint32_t c_MaxBindlessTextures{1024};
constexpr uint32_t c_SamplerCount{CI::c_SamplerCIs.size()};
constexpr uint32_t c_MinGuarenteedPushConstantSize{128};
constexpr uint32_t c_MaxFramesInFlight{2};
} // namespace Limits

} // namespace SYN::VK
