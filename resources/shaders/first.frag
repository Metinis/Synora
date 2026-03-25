#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(location = 0) in vec2 uv;

layout (location = 0) out vec4 outColor;

struct Vertex {
    vec3 pos;
    float u;
    float v;
};
layout(buffer_reference, std430) buffer readonly VertexBuffer {
    Vertex vertices[];
};

layout(push_constant, std430) uniform PushConstants {
    VertexBuffer vertexBuffer;
    uint textureIndex;
};

void main() {
    outColor = vec4(texture(textures[textureIndex], uv));
}
