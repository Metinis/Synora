#include <SynoraEngine/project/importers/AnimationImporter.h>

#include <spdlog/spdlog.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <SynoraEngine/project/AssetManager.h>

namespace SYN {
bool AnimationImporter::load(std::filesystem::path filepath,
                             AnimationClipData &asset,
                             AssetManager *assetManager) {
    Assimp::Importer importer;
    const aiScene *scene =
        importer.ReadFile(filepath.string(), aiProcess_LimitBoneWeights);

    if (scene == nullptr || scene->mRootNode == nullptr) {
        spdlog::warn("Could not load [{}]: {}", filepath.string(),
                     importer.GetErrorString());
        return false;
    }

    auto loadVectorKeys = [](aiVectorKey *keys, uint32_t count) {
        std::vector<VectorKey> outputKeys;

        for (uint32_t i = 0; i < count; ++i) {
            aiVector3D value = keys[i].mValue;
            outputKeys.emplace_back(glm::vec3(value.x, value.y, value.z),
                                    keys[i].mTime);
        }

        return outputKeys;
    };

    auto loadQuatKeys = [](aiQuatKey *keys, uint32_t count) {
        std::vector<QuatKey> outputKeys;

        for (uint32_t i = 0; i < count; ++i) {
            aiQuaternion value = keys[i].mValue;
            outputKeys.emplace_back(
                glm::quat(value.w, value.x, value.y, value.z), keys[i].mTime);
        }

        return outputKeys;
    };

    std::vector<AnimationClipData> clips;

    for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
        const aiAnimation *animation = scene->mAnimations[i];
        double fps = animation->mTicksPerSecond;

        // If ticks per second is unspecified in the loaded file format default
        // to 24 fps.
        if (fps == 0.0)
            fps = 24.0;

        AnimationClipData &clip = clips.emplace_back(animation->mName.C_Str(),
                                                     animation->mDuration, fps);

        for (uint32_t j = 0; j < animation->mNumChannels; ++j) {
            const aiNodeAnim *channel = animation->mChannels[j];
            clip.channels[channel->mNodeName.C_Str()] = AnimationChannel{
                loadVectorKeys(channel->mPositionKeys,
                               channel->mNumPositionKeys),
                loadVectorKeys(channel->mScalingKeys, channel->mNumScalingKeys),
                loadQuatKeys(channel->mRotationKeys, channel->mNumRotationKeys),
            };
        }
    }

    if (clips.empty()) {
        spdlog::warn("No clips found in [{}]. Empty clip data returned.",
                     filepath.string());
    } else {
        asset = clips[0];
    }

    for (uint32_t i = 1; i < clips.size(); ++i) {
        std::string key =
            std::format("{}|{}", filepath.stem().string(), clips[i].name);
        assetManager->add(std::move(clips[i]), key);
    }

    return true;
}
} // namespace SYN
