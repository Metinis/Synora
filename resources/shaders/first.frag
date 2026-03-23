#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;

void main() {
    outColor = vec4(texture(textures[1023], uv));
}
