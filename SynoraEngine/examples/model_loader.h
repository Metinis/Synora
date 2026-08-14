#pragma once

#include <SynoraEngine/gfx/gl/GL.h>

#include <filesystem>
#include <optional>

namespace SYN::gfx::gl {

std::optional<ModelData> loadModelData(const std::filesystem::path &path);
std::vector<AnimationClip>
loadAnimationClips(const std::filesystem::path &path);

} // namespace SYN::gfx::gl
