#include "SynoraEngine/scene/Components.h"
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

glm::mat4 TransformComp::getLocalMatrix() {
    glm::mat4 t = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 r = glm::toMat4(rotation);
    glm::mat4 s = glm::scale(glm::mat4(1.0f), scale);
    return t * r * s;
}
