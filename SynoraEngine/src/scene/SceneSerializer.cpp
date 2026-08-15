#include "SynoraEngine/file/SceneSerializer.h"
#include "SynoraEngine/scene/Components.h"
#include "SynoraEngine/scene/Scene.h" 
#include "SynoraEngine/scene/Entity.h"
#include "spdlog/spdlog.h"
#include "yaml-cpp/yaml.h"
#include <filesystem>
#include <fstream>

namespace SYN::SceneSerializer {
  static void serializeEntity(YAML::Emitter& out, Entity& entity) {
    out << YAML::BeginMap;
    out << YAML::Key << "Entity" << YAML::Value << entity.getComponent<UUIDComp>().id;
    if(auto* tag = entity.tryGetComponent<TagComp>()) {
      out << YAML::Key << "TagComp" << YAML::Value << tag->tag;
    }
    if (auto* parent = entity.tryGetComponent<ParentComp>()) {
      out << YAML::Key << "ParentComp";
      out << YAML::Value << YAML::BeginMap;
      out << YAML::Key << "id" << YAML::Value << parent->id;
      out << YAML::EndMap;
    }

    if (auto* mesh = entity.tryGetComponent<MeshComp>()) {
      out << YAML::Key << "MeshComp";
      out << YAML::Value << YAML::BeginMap;
      out << YAML::Key << "id" << YAML::Value << mesh->id;
      out << YAML::EndMap;
    }

    if (auto* material = entity.tryGetComponent<MaterialComp>()) {
      out << YAML::Key << "MaterialComp";
      out << YAML::Value << YAML::BeginMap;
      out << YAML::Key << "id" << YAML::Value << material->id;
      out << YAML::EndMap;
    }

    if (auto* transform = entity.tryGetComponent<TransformComp>()) {
      out << YAML::Key << "TransformComp";
      out << YAML::Value << YAML::BeginMap;

      out << YAML::Key << "position";
      out << YAML::Value << YAML::Flow
        << YAML::BeginSeq
        << transform->position.x
        << transform->position.y
        << transform->position.z
        << YAML::EndSeq;

      out << YAML::Key << "rotation";
      out << YAML::Value << YAML::Flow
        << YAML::BeginSeq
        << transform->rotation.w
        << transform->rotation.x
        << transform->rotation.y
        << transform->rotation.z
        << YAML::EndSeq;

      out << YAML::Key << "scale";
      out << YAML::Value << YAML::Flow
        << YAML::BeginSeq
        << transform->scale.x
        << transform->scale.y
        << transform->scale.z
        << YAML::EndSeq;


      out << YAML::Key << "depth" << YAML::Value << transform->depth;

      out << YAML::EndMap;
    }

    if (auto* camera = entity.tryGetComponent<CameraComp>()) {
      out << YAML::Key << "CameraComp";
      out << YAML::Value << YAML::BeginMap;

      out << YAML::Key << "fovDegrees"
        << YAML::Value << camera->fovDegrees;

      out << YAML::Key << "aspectRatio"
        << YAML::Value << camera->aspectRatio;

      out << YAML::Key << "nearPlane"
        << YAML::Value << camera->nearPlane;

      out << YAML::Key << "farPlane"
        << YAML::Value << camera->farPlane;

      out << YAML::Key << "isPrimary"
        << YAML::Value << camera->isPrimary;

      out << YAML::EndMap;
    }
    out << YAML::EndMap;
  }
static void deserializeEntity(const YAML::Node& node, Entity& entity) {
    // Entity UUID
    if (auto uuidNode = node["Entity"]) {
        auto& uuid = entity.addComponent<UUIDComp>();
        uuid.id = uuidNode.as<SYN::UUID>();
    }

    // Tag
    if (auto tagNode = node["TagComp"]) {
        auto& tag = entity.addComponent<TagComp>();
        tag.tag = tagNode.as<std::string>();
    }

    // Parent
    if (auto parentNode = node["ParentComp"]) {
        auto& parent = entity.addComponent<ParentComp>();

        if (auto idNode = parentNode["id"]) {
            parent.id = idNode.as<SYN::UUID>();
        }
    }

    // Transform
    if (auto transformNode = node["TransformComp"]) {
        auto& transform = entity.addComponent<TransformComp>();

        if (auto position = transformNode["position"]) {
            transform.position = {
                position[0].as<float>(),
                position[1].as<float>(),
                position[2].as<float>()
            };
        }

        if (auto rotation = transformNode["rotation"]) {
            transform.rotation = {
                rotation[0].as<float>(), // w
                rotation[1].as<float>(), // x
                rotation[2].as<float>(), // y
                rotation[3].as<float>()  // z
            };
        }

        if (auto scale = transformNode["scale"]) {
            transform.scale = {
                scale[0].as<float>(),
                scale[1].as<float>(),
                scale[2].as<float>()
            };
        }

        if (auto depth = transformNode["depth"]) {
            transform.depth = depth.as<int>();
        }

        // Rebuild cached values
        transform.eulerAngles =
            glm::degrees(glm::eulerAngles(transform.rotation));

        transform.setLocalMatrix(
            glm::translate(glm::mat4(1.0f), transform.position) *
            glm::mat4_cast(transform.rotation) *
            glm::scale(glm::mat4(1.0f), transform.scale)
        );
    }

    // Mesh
    if (auto meshNode = node["MeshComp"]) {
        if (auto idNode = meshNode["id"]) {
            MeshComp mesh = { .id =idNode.as<SYN::UUID>() };
            entity.addComponent<MeshComp>(mesh);
        }
    }

    // Material
    if (auto materialNode = node["MaterialComp"]) {
        if (auto idNode = materialNode["id"]) {
            MaterialComp mat = { .id =idNode.as<SYN::UUID>() };
            entity.addComponent<MaterialComp>(mat);
        }
    }

    // Camera
    if (auto cameraNode = node["CameraComp"]) {
        auto& camera = entity.addComponent<CameraComp>();

        camera.fovDegrees =
            cameraNode["fovDegrees"].as<float>(90.f);

        camera.aspectRatio =
            cameraNode["aspectRatio"].as<float>(16.f / 9.f);

        camera.nearPlane =
            cameraNode["nearPlane"].as<float>(0.1f);

        camera.farPlane =
            cameraNode["farPlane"].as<float>(100.f);

        camera.isPrimary =
            cameraNode["isPrimary"].as<bool>(false);
    }
}
  bool deserialize(SYN::Scene* scene, const std::filesystem::path& scenePath) {
    try {

      if (!std::filesystem::exists(scenePath)) {
        spdlog::error("Scene file does not exist: {}", scenePath.string());
        return false;
      }

      YAML::Node data = YAML::LoadFile(scenePath.string());

      auto sceneNode = data["Scene"];
      if (!sceneNode) {
        spdlog::error("Invalid scene file: missing 'Scene'");
        return false;
      }

      auto entitiesNode = data["Entities"];
      if (!entitiesNode || !entitiesNode.IsSequence()) {
        spdlog::error("Invalid scene file: missing 'Entities' sequence");
        return false;
      }

      scene->clear();
      for (const auto& entityNode : entitiesNode) {
        auto uuidNode = entityNode["Entity"];

        if (!uuidNode) {
          spdlog::warn("Skipping entity without UUID");
          continue;
        }

        auto entity = scene->createEmptyEntity();

        deserializeEntity(entityNode, entity);
      }


    } catch (const YAML::Exception& e) {
      spdlog::error(
          "Failed to parse scene at {}: {}",
          scenePath.string(),
          e.what()
          );
      return false;
    } catch (const std::exception& e) {
      spdlog::error(
          "Failed to load scene at {}: {}",
          scenePath.string(),
          e.what()
          );
      return false;
    } catch (...) {
      spdlog::error(
          "Failed to load scene at {}",
          scenePath.string()
          );
      return false;
    }


    spdlog::debug("Loaded scene at {}", scenePath.string());
    
    return true;
  }
  bool serialize(SYN::Scene* scene, const std::filesystem::path& path) {
    try {
      YAML::Emitter out;
      out << YAML::BeginMap;
      out << YAML::Key << "Scene" << YAML::Value << "Default";
      out << YAML::Key << "Entities" << YAML::BeginSeq;

      //every entity must have an UUID anyways
      for(auto& e : scene->getEntities<UUIDComp>()) {
        serializeEntity(out, e);
      }

      out << YAML::EndSeq;
      out << YAML::EndMap;

      std::ofstream fout(path / "scene.yaml");
      fout << out.c_str(); 
    } catch(...) {
      spdlog::error("Failed to save scene at {}", path.string());
      return false;
    }
    spdlog::debug("Saved scene at {}", path.string());

    return true;
  }
}
