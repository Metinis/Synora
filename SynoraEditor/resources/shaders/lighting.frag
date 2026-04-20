#version 450
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

layout(set = 0, binding = 0) uniform sampler2D textures[];
layout(set = 0, binding = 1) uniform samplerCube cubeMaps[];

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 fragPos;
layout(location = 2) in vec3 faceNormal;
layout(location = 3) in vec3 faceTangent;
layout(location = 4) in float handedness;

layout (location = 0) out vec4 outColor;

struct Vertex {
    vec3 pos;
    float u;
    vec3 normal;
    float v;
    vec4 tangent;
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
    uint albedoIndex;
    uint metallicRoughnessIndex;
    uint normalMap;
};


layout(set = 1, binding = 0) uniform Uniform {
    mat4 projectionMat;
    mat4 viewMat;
    vec3 cameraPos;
    uint nLights;
    LightCaster lights[16];
};

const float PI = 3.141592653589793;

float ggxNDF(float roughness, float nDotH) {
    float a2 = roughness * roughness;

    float denom = 1.f + (nDotH * nDotH) * (a2 - 1.f);
    denom = PI * denom * denom;
    return a2 / denom;
}

// height correlated Smith G2 for ggx + normalization factor for brdf
// equal to ggx
float ggxVisibility(float roughness, float nDotL, float nDotV) {
    float uo = nDotV;
    float ui = nDotL;

    float a2 = roughness * roughness;

    float ui2 = ui * ui;
    float uo2 = uo * uo;

    float denom = uo * sqrt(a2 + ui2 * (1.f - a2));
    denom += ui * sqrt(a2 + uo2 * (1.f - a2));

    return 0.5f / denom;

}

void main() {
    vec3 tangent = normalize(faceTangent - dot(faceTangent, faceNormal) * faceNormal);
    vec3 bitangent = cross(faceNormal, faceTangent) * handedness;

    mat3 tbn = mat3(tangent, bitangent, faceNormal);

    vec3 mapNormal = texture(textures[normalMap], uv).xyz;
    mapNormal = (mapNormal * 2.f) - 1.f;

    vec3 norm = normalize(tbn * mapNormal);

    vec4 fragAlbedo = vec4(texture(textures[albedoIndex], uv));
    vec4 fragMetallicRoughness = vec4(texture(textures[metallicRoughnessIndex], uv));

    vec3 albedo = fragAlbedo.xyz;
    float roughness = fragMetallicRoughness.g;
    float metallic = fragMetallicRoughness.b;

    if (fragAlbedo.a == 0.f) {
        discard;
    }

    vec3 lo = vec3(0.f, 0.f, 0.f);
    for (int i = 0; i < nLights; i++) {
        LightCaster light = lights[i];
        float lightDistance = distance(light.pos, fragPos);

        vec3 lightDir = normalize(light.pos - fragPos);
        vec3 viewDir = normalize(cameraPos - fragPos);

        vec3 halfway = normalize(lightDir + viewDir);

        float nDotL = abs(dot(norm, lightDir)) + 0.00001f;
        float vDotH = max(dot(viewDir, halfway), 0.f);
        float nDotH = abs(dot(norm, halfway));
        float nDotV = abs(dot(norm, viewDir));

        vec3 f0 = mix(vec3(0.04f), albedo, metallic);

        float r = roughness * roughness;

        float D = ggxNDF(r, nDotH);
        float G = ggxVisibility(r, nDotL, nDotV);
        vec3 F = f0 + (1.f - f0) * pow(1.f - vDotH, 5.f);

        vec3 specular = D * G * F; // normalization is in G
        vec3 diffuse = ((1.f - metallic) * (1.f - F) * albedo) / PI;

        vec3 brdf = specular + diffuse;

        lo += brdf * light.color * nDotL;
    }

    vec3 skyColor = vec3(0.2f, 0.2f, 0.3f);
    vec3 groundColor = vec3(0.1f, 0.07f, 0.05f);

    float hemisphere = dot(norm, vec3(0.f, 1.f, 0.f)) * 0.5f + 0.5f;
    vec3 ambient = fragAlbedo.xyz * mix(groundColor, skyColor, hemisphere);

    vec3 color = ambient + lo;
    color = color / (color + vec3(1.f));

    outColor = vec4(color, 1.f);
}
