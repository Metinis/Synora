#pragma once
#include "SynoraEngine/scene/Scene.h"

namespace SYN::SceneSerializer {
  bool deserialize(SYN::Scene* scene, const std::filesystem::path& path);
  bool serialize(SYN::Scene* scene, const std::filesystem::path& path);
}
