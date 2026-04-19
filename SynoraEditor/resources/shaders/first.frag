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

const float PI = 3.141592653589793;

void main() {
    vec4 fragAlbedo = vec4(texture(textures[textureIndex], uv));
    if (fragAlbedo.a == 0.f) {
        discard;
    }

    vec3 fragColor = vec3(0.f, 0.f, 0.f);
    for (int i = 0; i < nLights; i++) {
        LightCaster light = lights[i];

        vec3 lightDir = normalize(light.pos - fragPos);
        vec3 viewDir = normalize(cameraPos - fragPos);

        vec3 halfway = normalize(lightDir + viewDir);

        float nDotL = max(dot(norm, lightDir), 0.f);
        float vDotH = max(dot(viewDir, halfway), 0.f);

        float shinyness = 8.f;
        float specularNormalizationFactor = (shinyness + 2.f) / (2.f * PI);

        float specularFactor = pow(max(dot(norm, halfway), 0.f), shinyness) * specularNormalizationFactor;

        vec3 specular = vec3(specularFactor);
        vec3 diffuse = fragAlbedo.xyz  / PI;

        float f0 = 0.04f;
        float f = f0 + (1.f - f0) * pow(1.f - vDotH, 5.f);

        vec3 brdf = specular * f + diffuse * (1.f - f);

        vec3 ambient = fragAlbedo.xyz * 0.1f;

        fragColor += (brdf * light.color * nDotL) + ambient;
    }

    fragColor *= PI;

    outColor = vec4(fragColor, 0.f);
}
