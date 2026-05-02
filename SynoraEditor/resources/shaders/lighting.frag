#version 450
#include "Bindless.glsl"
#include "BRDF.glsl"

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
    uint testIndex;
};


layout(set = 1, binding = 0) uniform Uniform {
    mat4 projectionMat;
    mat4 viewMat;
    vec3 cameraPos;
    uint nLights;
    LightCaster lights[16];
};

float hash2D(float x, float y) {
    return fract(1.0e4 * sin(17.0 * x + 0.1 * y) * (0.1 +abs(sin(13.0 * y + x))));
}

float hash3D(float x, float y, float z) { return hash2D(hash2D(x,y),z); }

void main() {
    vec3 N = normalize(faceNormal);
    vec3 T = normalize(faceTangent);

    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T) * handedness;

    mat3 tbn = mat3(T, B, N);

    vec3 mapNormal = sampleLinearR(normalMap, uv).xyz;
    mapNormal = normalize((mapNormal * 2.f) - 1.f);

    vec3 norm = normalize(tbn * mapNormal);

    vec4 fragAlbedo = sampleLinearR(albedoIndex, uv);
    vec4 fragMetallicRoughness = sampleLinearR(metallicRoughnessIndex, uv);

    vec3 albedo = fragAlbedo.xyz;
    float roughness = fragMetallicRoughness.g;
    float metallic = fragMetallicRoughness.b;

    vec3 maxDFragPos = max(dFdx(fragPos), dFdy(fragPos));

    vec3 ssFragPos = fragPos / length(maxDFragPos);
    ssFragPos = clamp(ssFragPos, 0.f, 1.f);
    float rand = hash3D(ssFragPos.x, ssFragPos.y, ssFragPos.z);

    if (fragAlbedo.a < rand || fragAlbedo.a == 0.f) {
        discard;
    }

    vec3 lo = vec3(0.f, 0.f, 0.f);
    for (int i = 0; i < nLights; i++) {
        LightCaster light = lights[i];
        float lightDistance = distance(light.pos, fragPos);
        float attenuation = 1.f / (lightDistance * lightDistance);

        vec3 lightDir = normalize(light.pos - fragPos);
        vec3 viewDir = normalize(cameraPos - fragPos);

        vec3 halfway = normalize(lightDir + viewDir);

        float nDotL = max(abs(dot(norm, lightDir)) + 0.0001f, 0.f);
        float vDotH = max(abs(dot(viewDir, halfway)) + 0.0001f, 0.f);
        float nDotH = abs(dot(norm, halfway));
        float nDotV = abs(dot(norm, viewDir));

        vec3 F0 = mix(vec3(0.04f), albedo, metallic);

        float r = roughness * roughness;

        float RsF1L = sampleLinear(testIndex, vec2(r, nDotL)).x;
        float RsF1V = sampleLinear(testIndex, vec2(r, nDotV)).x;
        float barRsF1 = sampleLinear(testIndex, vec2(r, 0.5f)).y;
        vec3 barF = ((20.f/21.f) * F0) + (1.f / 21.f);

        vec3 ms = (barF * barRsF1) * (1.f - RsF1L) * (1.f - RsF1V);
        vec3 denom = (PI * (1.f - barRsF1) * (1.f - (barF * (1.f - barRsF1))));
        ms /= max(denom, 1e-6);
        
        float D = ggxNDF(r, nDotH);
        float G = ggxVisibility(r, nDotL, nDotV);
        vec3 F = F0 + (1.f - F0) * pow(1.f - vDotH, 5.f);

        vec3 specular = (D * G * F) + ms; // normalization is in G
        vec3 diffuse = ((1.f - metallic) * (1.f - F) * albedo) / PI;

        vec3 brdf = diffuse + specular;

        lo += brdf * light.color * attenuation * nDotL;
    }

    vec3 skyColor = vec3(0.2f, 0.2f, 0.3f);
    vec3 groundColor = vec3(0.1f, 0.07f, 0.05f);

    float hemisphere = dot(norm, vec3(0.f, 1.f, 0.f)) * 0.5f + 0.5f;
    vec3 ambient = fragAlbedo.xyz * mix(groundColor, skyColor, hemisphere);

    vec3 color = lo;

    outColor = vec4(color, 1.f);
}
