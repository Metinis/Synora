#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

namespace SYN {
struct VectorKey {
    glm::vec3 value;
    float timestamp;
};

struct QuatKey {
    glm::quat value;
    float timestamp;
};

struct AnimationChannel {
    std::vector<VectorKey> translation;
    std::vector<VectorKey> scale;
    std::vector<QuatKey> rotation;
};

struct AnimationClipInstance {
    std::string name;

    float duration;
    float fps;

    // Bone name -> Animation Channel
    std::unordered_map<std::string, AnimationChannel> channels;
};

struct AnimationClipData {
    std::vector<AnimationClipInstance> clips;
};

} // namespace SYN
