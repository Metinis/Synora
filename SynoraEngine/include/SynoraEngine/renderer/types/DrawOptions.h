#pragma once

#include <glm/mat4x4.hpp>

#include <SynoraEngine/project/UUID.h>
#include <SynoraEngine/scene/components/CameraComponent.h>

#include "Pass.h"

namespace SYN {
struct DrawOptions {
    using RenderTarget = std::variant<std::string, UUID>;
    CameraComponent camera;
    glm::mat4 cameraTransform = glm::mat4(1.0f);
    Pass passInfo;
};
} // namespace SYN
