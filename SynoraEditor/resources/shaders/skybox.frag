#version 450
#include "Bindless.glsl"

layout(location = 0) in vec3 lookDir;

layout (location = 0) out vec4 outColor;

layout(push_constant, std430) uniform PushConstants {
    uint cubeMapIndex;
};

layout(set = 1, binding = 0) uniform Uniform {
    mat4 invProjectionViewMat;
};

void main() {
    outColor = sampleLinear(cubeMapIndex, normalize(lookDir));
}
