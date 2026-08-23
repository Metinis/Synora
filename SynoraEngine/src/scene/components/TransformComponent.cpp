#include "SynoraEngine/scene/components/TransformComponent.h"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"
#include <glm/gtx/matrix_decompose.hpp>

namespace SYN {
glm::mat4 TransformComponent::getLocalMatrix() const {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 r = glm::toMat4(rotation);
    glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
    return t * r * s;
}
void TransformComponent::setLocalMatrix(glm::mat4 m) {
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(m, scale, rotation, position, skew, perspective);
    rotation = glm::normalize(rotation);
}
} // namespace SYN
