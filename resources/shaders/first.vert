#version 450
#extension GL_EXT_buffer_reference : require // needed for vertex pulling
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];

struct Vertex {
    vec3 pos;
};

layout(buffer_reference, std430) buffer readonly VertexBuffer {
    Vertex vertices[];
};

layout(push_constant, std430) uniform PushConstants {
    VertexBuffer vertexBuffer;
};

void main() {
	gl_Position = vec4(vertexBuffer.vertices[gl_VertexIndex].pos, 1.f);
}
