#include "SynoraEngine/file/SceneSerializer.h"
#include "SynoraEngine/scene/Scene.h" 
#include "spdlog/spdlog.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <fstream>

namespace SYN::SceneSerializer {
  bool serialize(SYN::Scene* scene, const std::filesystem::path& path) {
    YAML::Node sceneNode;

    sceneNode["Scene Name"] = "Default Scene";

    std::ofstream fout(path / "scene.yaml");
    spdlog::debug("Saved scene at {}", path.string());

    return false;
  }
}
