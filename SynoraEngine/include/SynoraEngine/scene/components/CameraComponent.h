#pragma once

namespace SYN {
struct CameraComponent {
    float fovDegrees = 90.f;
    float aspectRatio = 16.f / 9.f;
    float nearPlane = 0.1;
    float farPlane = 100.f;
    bool isPrimary;
};

// Attach with CameraComponent
struct FlyCameraComponent {
    bool tag;
};
} // namespace SYN
