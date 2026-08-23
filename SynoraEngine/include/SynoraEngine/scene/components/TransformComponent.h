#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace SYN {
struct TransformComponent {
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
    glm::vec3 scale{1.f, 1.f, 1.f};

    glm::mat4 getLocalMatrix() const;
    void setLocalMatrix(glm::mat4 m);
};
} // namespace SYN
