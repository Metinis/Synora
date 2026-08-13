#pragma once
#include "SynoraEngine/scene/Scene.h"

namespace SYN::SceneSerializer {
  bool serialize(SYN::Scene* scene, const std::filesystem::path& path);
}
