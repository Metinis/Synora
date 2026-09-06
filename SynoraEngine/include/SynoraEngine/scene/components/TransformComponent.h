#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace SYN {
struct TransformComponent {
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.f, 0.f, 0.f, 0.f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 getLocalMatrix() const;
    void setLocalMatrix(glm::mat4 m);
};
} // namespace SYN
