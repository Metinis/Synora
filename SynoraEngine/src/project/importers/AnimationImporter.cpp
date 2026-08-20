#include <SynoraEngine/project/importers/AnimationImporter.h>

#include <spdlog/spdlog.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <SynoraEngine/project/AssetManager.h>

namespace SYN {

namespace {
AnimationClipData loadAnimationClip(const aiAnimation *animation) {
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

    double fps = animation->mTicksPerSecond;

    // If ticks per second is unspecified in the loaded file format default
    // to 24 fps.
    if (fps == 0.0)
        fps = 24.0;

    AnimationClipData clip(animation->mName.C_Str(), animation->mDuration, fps);

    for (uint32_t j = 0; j < animation->mNumChannels; ++j) {
        const aiNodeAnim *channel = animation->mChannels[j];
        clip.channels[channel->mNodeName.C_Str()] = AnimationChannel{
            loadVectorKeys(channel->mPositionKeys, channel->mNumPositionKeys),
            loadVectorKeys(channel->mScalingKeys, channel->mNumScalingKeys),
            loadQuatKeys(channel->mRotationKeys, channel->mNumRotationKeys),
        };
    }

    return clip;
}
} // namespace

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

    if (scene->mNumAnimations == 0) {
        spdlog::warn("No clips found in [{}].", filepath.string());
        return false;
    }

    if (scene->mNumAnimations != 1) {
        spdlog::warn(
            "Multiple clips found in [{}]. Please use 'loadGroup' instead.",
            filepath.string());
        return false;
    }

    asset = loadAnimationClip(scene->mAnimations[0]);

    return true;
}

bool AnimationImporter::loadGroup(
    std::filesystem::path filepath,
    std::vector<std::pair<std::string, AnimationClipData>> &assets,
    class AssetManager *assetManager) {
    Assimp::Importer importer;
    const aiScene *scene =
        importer.ReadFile(filepath.string(), aiProcess_LimitBoneWeights);

    if (scene == nullptr || scene->mRootNode == nullptr) {
        spdlog::warn("Could not load [{}]: {}", filepath.string(),
                     importer.GetErrorString());
        return false;
    }

    if (scene->mNumAnimations == 0) {
        spdlog::warn("No clips found in [{}].", filepath.string());
        return false;
    }

    for (uint32_t i = 0; i < scene->mNumAnimations; ++i) {
        const aiAnimation *animation = scene->mAnimations[i];
        std::string key = std::format("{}|{}", filepath.stem().string(),
                                      animation->mName.C_Str());
        assets.emplace_back(
            std::make_pair(std::move(key), loadAnimationClip(animation)));
    }

    return true;
}
} // namespace SYN
