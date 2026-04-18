#version 450
#extension GL_EXT_buffer_reference : require // needed for vertex pulling
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 1) uniform samplerCube cubeMaps[];

layout(location = 0) out vec2 outUV;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outNorm;

struct Vertex {
    vec3 pos;
    float u;
    vec3 normal;
    float v;
};

struct LightCaster {
    vec3 pos;
    float padding0;
    vec3 color;
    float padding1;
    vec3 dir;
    float padding2;
};

layout(buffer_reference, std430) buffer readonly VertexBuffer {
    Vertex vertices[];
};

layout(buffer_reference, std430) buffer readonly IndexBuffer {
    uint indices[];
};


layout(push_constant, std430) uniform PushConstants {
    mat4 modelMat;
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    uint textureIndex;
};


layout(set = 1, binding = 0) uniform Uniform {
    mat4 projectionMat;
    mat4 viewMat;
    vec3 cameraPos;
    uint nLights;
    LightCaster lights[16];
};

void main() {
    Vertex vertex = vertexBuffer.vertices[indexBuffer.indices[gl_VertexIndex]];

    outUV = vec2(vertex.u, vertex.v);
    vec4 worldPos = modelMat * vec4(vertex.pos.xyz, 1.f);
    outWorldPos = worldPos.xyz;

    mat3 normalMat = transpose(mat3(modelMat));
    outNorm = normalize(normalMat * vertex.normal);

	gl_Position = projectionMat * viewMat * worldPos;
}
