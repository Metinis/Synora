#version 450 
#include "Bindless.glsl"

layout(location = 0) out vec3 outLookDir;

layout(push_constant, std430) uniform PushConstants {
    uint cubeMapIndex;
};

layout(set = 1, binding = 0) uniform Uniform {
    mat4 invProjectionViewMat;
};

vec2 vertices[6] = {
    vec2(-1.f, -1.f),
    vec2(1.f, -1.f),
    vec2(1.f, 1.f),

    vec2(1.f, 1.f),
    vec2(-1.f, 1.f),
    vec2(-1.f, -1.f),
};

void main() {
    vec2 uv = vertices[gl_VertexIndex];
    vec4 clipPos = vec4(uv, 1.f, 1.f);
    vec4 worldDir = invProjectionViewMat * clipPos;
    outLookDir = worldDir.xyz;

	gl_Position = vec4(uv, 1.f, 1.f);
}
