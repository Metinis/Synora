#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 1) uniform samplerCube cubeMaps[];

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 norm;

layout (location = 0) out vec4 outColor;

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
    mat4 modelMatrix;
    VertexBuffer vertexBuffer;
    IndexBuffer indexBuffer;
    uint textureIndex;
};


layout(set = 1, binding = 0) uniform Uniform {
    mat4 projectionMat;
    mat4 viewMat;
    LightCaster lights[16];
    uint nLights;
};

void main() {
    vec4 fragAlbedo = vec4(texture(textures[textureIndex], uv));
    if (fragAlbedo.a == 0.f) {
        discard;
    }

    outColor = vec4(0.f, 0.f, 0.f, 0.f);

    for (int i = 0; i < nLights; i++) {
        LightCaster light = lights[i];

        vec3 lightPos = vec4(viewMat * vec4(light.pos, 1.f)).xyz;

        vec3 lightDir = normalize(lightPos - fragPos);
        vec3 viewDir = normalize(-fragPos);

        vec3 halfway = normalize(lightDir + viewDir);

        vec3 ambient = fragAlbedo.xyz * 0.1f;
        vec3 diffuse = fragAlbedo.xyz * max(dot(norm, lightDir), 0.f) * 0.7;

        vec3 specular = fragAlbedo.xyz * pow(max(dot(norm, halfway), 0.f), 16.f);

        outColor += vec4(specular + diffuse + ambient, 1.f);
    }
}
