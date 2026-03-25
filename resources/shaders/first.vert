#version 450
#extension GL_EXT_buffer_reference : require // needed for vertex pulling
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(location = 0) out vec2 outUV;

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

layout(set = 1, binding = 0) uniform Uniform {
    mat4 modelMatrix;
};

void main() {
    Vertex vertex = vertexBuffer.vertices[gl_VertexIndex];

    outUV = vec2(vertex.u, vertex.v);
	gl_Position = modelMatrix * vec4(vertex.pos.xyz, 1.f);
}
