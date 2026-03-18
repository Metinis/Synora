#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];

layout (location = 0) out vec4 outColor;

void main() {
    outColor = vec4(1.f, 0.f, 1.0f, 1.f);
}
