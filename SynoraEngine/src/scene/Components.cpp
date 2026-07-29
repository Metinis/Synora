#include "SynoraEngine/scene/Components.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"
#include <glm/gtx/matrix_decompose.hpp>

glm::mat4 TransformComp::getLocalMatrix() {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 r = glm::toMat4(rotation);
    glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
    return t * r * s;
}
void TransformComp::setLocalMatrix(glm::mat4 m) {
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(m, scale, rotation, position, skew, perspective);
    rotation = glm::normalize(rotation);
}
