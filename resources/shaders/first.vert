#version 450
#extension GL_EXT_buffer_reference : require // needed for vertex pulling
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 1) uniform samplerCube cubeMaps[];

layout(location = 0) out vec2 outUV;

struct Vertex {
    vec3 pos;
    float u;
    vec3 normal;
    float v;
};
layout(buffer_reference, std430) buffer readonly VertexBuffer {
    Vertex vertices[];
};

layout(buffer_reference, std430) buffer readonly IndexBuffer {
    uint indices[];
};


layout(push_constant, std430) uniform PushConstants {
    mat4 modelMatrix;
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    uint textureIndex;
};

layout(set = 1, binding = 0) uniform Uniform {
    mat4 projectionViewMat;
};

void main() {
    Vertex vertex = vertexBuffer.vertices[indexBuffer.indices[gl_VertexIndex]];

    outUV = vec2(vertex.u, vertex.v);
	gl_Position = projectionViewMat * modelMatrix * vec4(vertex.pos.xyz, 1.f);
}
