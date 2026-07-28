#version 450 core
out vec4 fragColor;
in vec3 fragTexCoords;

uniform samplerCube u_hdrMap;
uniform float u_roughness;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits) {
  bits = (bits << 16u) | (bits >> 16u);
  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
  return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

vec2 Hammersley(uint i, uint N) {
  return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
  float a = roughness * roughness;

  float phi = 2.0 * PI * Xi.x;
  float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
  float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

  // from spherical coordinates to cartesian coordinates
  vec3 H;
  H.x = cos(phi) * sinTheta;
  H.y = sin(phi) * sinTheta;
  H.z = cosTheta;

  // from tangent-space vector to world-space sample vector
  vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 tangent = normalize(cross(up, N));
  vec3 bitangent = cross(N, tangent);

  vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
  return normalize(sampleVec);
}

float DistributionGGX(float NdotH, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float d = (NdotH * NdotH) * (a2 - 1.0) + 1.0;
  return a2 / (PI * d * d);
}

void main() {
  vec3 normal = normalize(fragTexCoords);
  vec3 viewDir = normal;

  const uint SAMPLE_COUNT = 1024u;
  float totalWeight = 0.0;
  vec3 prefilteredColor = vec3(0.0);

  float resolution = textureSize(u_hdrMap, 0).x;
  float saTexel = 4.0 * PI / (6.0 * resolution * resolution);

  for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
    vec2 Xi = Hammersley(i, SAMPLE_COUNT);
    vec3 halfwayDir = ImportanceSampleGGX(Xi, normal, u_roughness);
    vec3 lightDir = normalize(2.0 * dot(viewDir, halfwayDir) * halfwayDir - viewDir);

    float NdotL = max(dot(normal, lightDir), 0.0);
    if (NdotL > 0.0) {
      float NdotH = max(dot(normal, halfwayDir), 0.0);
      float HdotV = max(dot(halfwayDir, viewDir), 0.0);
      float D = DistributionGGX(NdotH, u_roughness);
      float pdf = D * 0.25f;

      float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf);
      float mipLevel = u_roughness == 0.0 ? 0.0 : max(0.5 * log2(saSample / saTexel), 0.0);
      prefilteredColor += textureLod(u_hdrMap, lightDir, mipLevel).rgb * NdotL;

      totalWeight += NdotL;
    }
  }
  prefilteredColor = prefilteredColor / max(totalWeight, 0.001f);

  fragColor = vec4(prefilteredColor, 1.0);
}
