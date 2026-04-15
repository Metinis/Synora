#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 1) uniform samplerCube cubeMaps[];

layout(location = 0) in vec3 lookDir;

layout (location = 0) out vec4 outColor;

layout(push_constant, std430) uniform PushConstants {
    uint cubeMapIndex;
};

layout(set = 1, binding = 0) uniform Uniform {
    mat4 invProjectionViewMat;
};

void main() {
    outColor = vec4(texture(cubeMaps[cubeMapIndex], normalize(lookDir)));
}
