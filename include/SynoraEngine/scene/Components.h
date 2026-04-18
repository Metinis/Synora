#pragma once
#include "SynoraEngine/project/UUID.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// id is used to access Data through asset manager
struct ParentComp {
    SYN::UUID id;
};
struct MeshComp {
    SYN::UUID id;
};
struct UUIDComp {
    SYN::UUID id;
};
struct TagComp {
    std::string tag{"Unnamed Entity"};
};
struct TransformComp {
    glm::mat4 worldMatrix;
    glm::mat4 getLocalMatrix();
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::quat rotation{0.f, 0.f, 0.f, 0.f};
    glm::vec3 scale{1.f, 1.f, 1.f};
    glm::vec3 eulerAngles =
        glm::degrees(glm::eulerAngles(rotation)); // cache for inspector
    int depth = 0;
};
struct MaterialComp {
    SYN::UUID id;
};
struct CameraComp {
    float fovDegrees = 90.f;
    float aspectRatio = 16.f / 9.f;
    float nearPlane = 0.1;
    float farPlane = 100.f;
    bool isPrimary;
};
