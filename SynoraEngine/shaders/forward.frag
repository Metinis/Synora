out vec4 fragColor;

in vec3 fragNormal;
in vec3 fragPos;
in vec2 fragTexCoords;

uniform vec3 u_tint;
uniform float u_roughness;
uniform float u_metallic;

layout(binding = 0) uniform sampler2D u_albedoTexture;
#ifdef FEATURE_NORMAL
layout(binding = 1) uniform sampler2D u_normalMap;
in mat3 TBN;
#endif

#ifdef FEATURE_METALLIC_ROUGHNESS
layout(binding = 2) uniform sampler2D u_metallicRoughness;
#endif

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 8.0;

layout(std140, binding = 0) uniform CameraConstants {
  mat4 u_ViewProjection;
  mat4 u_View;
  vec3 u_cameraPos;
};

#define CASCADE_COUNT 4
#define NEAR_CASCADE_COUNT 2
layout(std140, binding = 1) uniform ShadowConstants {
  mat4 u_lightSpaceMatrices[CASCADE_COUNT];
  vec4 u_cascadePlaneDistances;
  vec4 u_cascadeTexelWorldSize;
};

struct DirectionalLight {
  vec3 direction;
  vec3 color;
  float intensity;
  bool castShadow;
};
layout(std140, binding = 2) uniform LightConstants {
  DirectionalLight u_light;
};

layout(binding = 3) uniform samplerCube u_irradianceMap;
layout(binding = 4) uniform samplerCube u_prefilterMap;
layout(binding = 5) uniform sampler2D u_brdfLUT;
layout(binding = 6) uniform sampler2DArrayShadow u_csmNear;
layout(binding = 7) uniform sampler2DArrayShadow u_csmFar;

vec3 fresnelSchlick(float cosTheta, vec3 F0) {
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;

  float num = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;

  return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;

  float num = NdotV;
  float denom = NdotV * (1.0 - k) + k;

  return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx2 = GeometrySchlickGGX(NdotV, roughness);
  float ggx1 = GeometrySchlickGGX(NdotL, roughness);

  return ggx1 * ggx2;
}

// No variance without normal map
vec3 getNormalAvg() {
  #ifdef FEATURE_NORMAL
  vec3 mapNormal = texture(u_normalMap, fragTexCoords).xyz * 2.f - 1.f;
  vec3 normal = mapNormal;
  #else
  vec3 normal = normalize(fragNormal);
  #endif

  return normal;
}

vec3 getNormal() {
  #ifdef FEATURE_NORMAL
  vec3 mapNormal = texture(u_normalMap, fragTexCoords).xyz * 2.f - 1.f;
  vec3 normal = normalize(TBN * mapNormal);
  #else
  vec3 normal = normalize(fragNormal);
  #endif

  return normal;
}

float roughnessAA(vec3 normal, float roughness) {
  float sigma = 0.5;
  float sigma2 = sigma * sigma;
  float kappa = 0.4;

  vec3 dndu = dFdx(normal), dndv = dFdy(normal);

  float variance = sigma2 * (dot(dndu, dndu) + dot(dndv, dndv));

  vec3 Na = getNormalAvg();
  float toksvigVariance = 1 - dot(Na, Na);

  float roughness2 = roughness * roughness;
  float kernelRoughness2 = min(2.0 * variance + toksvigVariance, kappa);

  return sqrt(clamp(roughness2 + kernelRoughness2, 0.0, 1.0));
}

vec2 getMetallicRoughness() {
  float roughness = u_roughness;
  float metallic = u_metallic;

  #ifdef FEATURE_METALLIC_ROUGHNESS
  roughness *= texture(u_metallicRoughness, fragTexCoords).g;
  metallic *= texture(u_metallicRoughness, fragTexCoords).b;
  #endif

  roughness = clamp(roughness, 0.08, 1.0);

  return vec2(metallic, roughness);
}

vec2 poissonDisk[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790)
  );

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i) {
  vec4 seed4 = vec4(seed, i);
  float dot_product = dot(seed4, vec4(12.9898, 78.233, 45.164, 94.673));
  return fract(sin(dot_product) * 43758.5453);
}

int getCascadeLayer() {
  vec4 fragPosViewSpace = u_View * vec4(fragPos, 1.0);
  float depth = abs(fragPosViewSpace.z);

  int layer = CASCADE_COUNT - 1;
  for (int i = 0; i < CASCADE_COUNT; ++i) {
    if (depth < u_cascadePlaneDistances[i]) {
      layer = i;
      break;
    }
  }
  return layer;
}

float sampleCascade(vec3 lightDir, int layer, float texelWorldSize) {
  vec3 normal = normalize(fragNormal);

  float ndotl = clamp(dot(normal, lightDir), 0.0f, 1.0f);
  vec3 normalBias = normal * (1.0 - (ndotl * ndotl));
  vec4 lightSpaceFragPos = u_lightSpaceMatrices[layer] * vec4(fragPos + normalBias * texelWorldSize * 5.0f, 1.0);

  vec3 p = lightSpaceFragPos.xyz / lightSpaceFragPos.w;
  p = p * 0.5 + 0.5;

  if (p.z > 1.0) return 0.0f;

  float currentDepth = p.z;

  int maxSamples = 8;
  int nrSamples = 0;
  float shadow = 0.0f;

  vec2 shadowmapSize = (layer < NEAR_CASCADE_COUNT) ?
    vec2(textureSize(u_csmNear, 0)) :
    vec2(textureSize(u_csmFar, 0));

  vec2 texelSize = 1.0 / shadowmapSize;

  float spread = 0.005 / texelWorldSize;
  float angle = random(floor(fragPos.xyz * 1000.0), 0) * 2.0 * PI;
  float s = sin(angle), c = cos(angle);

  for (int i = 0; i < maxSamples; ++i) {
    ++nrSamples;
    vec2 rotatedDisk = vec2(
        poissonDisk[i].x * c - poissonDisk[i].y * s,
        poissonDisk[i].x * s + poissonDisk[i].y * c
      );
    vec2 uv = p.xy + rotatedDisk * spread * texelSize;

    float depthSample = (layer < NEAR_CASCADE_COUNT) ?
      texture(u_csmNear, vec4(uv, layer, currentDepth)) :
      texture(u_csmFar, vec4(uv, layer - NEAR_CASCADE_COUNT, currentDepth));

    float result = 1.0 - depthSample;
    shadow += result;
    if (i == 3 && (shadow == 0 || shadow == 4.0f)) break;
  }

  shadow /= nrSamples;

  return shadow;
}

float getShadow(vec3 lightDir) {
  int currentLayer = getCascadeLayer();
  vec4 fragPosViewSpace = u_View * vec4(fragPos, 1.0);
  float depth = abs(fragPosViewSpace.z);

  float splitStart = (currentLayer == 0) ? 0.0 : u_cascadePlaneDistances[currentLayer - 1];
  float splitEnd = u_cascadePlaneDistances[currentLayer];

  float band = (splitEnd - splitStart) * 0.15;

  float blend = smoothstep(splitEnd - band, splitEnd, depth);

  float ign = fract(52.9829189 * fract(dot(gl_FragCoord.xy, vec2(0.06711056, 0.00583715))));
  int nextLayer = min(currentLayer + 1, CASCADE_COUNT - 1);
  int chosenLayer = (ign < blend) ? nextLayer : currentLayer;
  float shadow = sampleCascade(lightDir, chosenLayer, u_cascadeTexelWorldSize[chosenLayer]);

  return shadow;
}

vec3 applyDirectionalLight(DirectionalLight light, vec3 objectColor, vec3 normal, vec2 metallicRoughness) {
  vec3 lightDir = -light.direction;
  vec3 viewDir = normalize(u_cameraPos - fragPos);
  vec3 halfwayDir = normalize(lightDir + viewDir);
  vec3 radiance = light.color * light.intensity;
  float NdotL = max(dot(normal, lightDir), 0.0);
  float metallic = metallicRoughness.x;
  float roughness = metallicRoughness.y;

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, objectColor, metallic);

  float NDF = DistributionGGX(normal, halfwayDir, roughness);
  float G = GeometrySmith(normal, viewDir, lightDir, roughness);
  vec3 F = fresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);

  vec3 kS = F;
  vec3 kD = vec3(1.0) - kS;
  kD *= 1.0 - metallic;

  vec3 numerator = NDF * G * F;
  float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
  vec3 specular = numerator / denominator;

  return (kD * objectColor / PI + specular) * radiance * NdotL * (1.0f - getShadow(lightDir));
}

vec3 getAmbientColor(vec3 objectColor, vec3 normal, vec2 metallicRoughness) {
  vec3 viewDir = normalize(u_cameraPos - fragPos);

  float metallic = metallicRoughness.x;
  float roughness = metallicRoughness.y;

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, objectColor, metallic);

  vec3 reflectDir = reflect(-viewDir, normal);
  vec3 radiance = textureLod(u_prefilterMap, reflectDir, roughness * MAX_REFLECTION_LOD).rgb;
  vec3 irradiance = texture(u_irradianceMap, normal).rgb;

  vec3 F = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);

  vec2 envBRDF = texture(u_brdfLUT, vec2(max(dot(normal, viewDir), 0.0), roughness)).rg;

  float ems = 1.0 - (envBRDF.x + envBRDF.y);
  vec3 fssEss = F * envBRDF.x + envBRDF.y;
  vec3 fAvg = F0 + (1.0 - F0) / 21.0;
  vec3 fmsEms = ems * fssEss * fAvg / (1.0 - fAvg * ems);
  vec3 kd = objectColor * (1.0 - metallic) * (1.0 - fssEss - fmsEms);

  return fssEss * radiance + (kd + fmsEms) * irradiance;
}

void main() {
  vec4 albedoTexture = texture(u_albedoTexture, fragTexCoords);
  vec3 objectColor = vec3(vec4(u_tint, 1.0) * albedoTexture);

  vec3 normal = getNormal();
  vec2 metallicRoughness = getMetallicRoughness();
  metallicRoughness.y = roughnessAA(normal, metallicRoughness.y);

  vec3 Lo = applyDirectionalLight(u_light, objectColor, normal, metallicRoughness);

  vec3 cascadeColors[4] = {
      vec3(1.0, 0.0, 0.0) * 0.1f,
      vec3(0.0, 1.0, 0.0) * 0.1f,
      vec3(0.0, 0.0, 1.0) * 0.1f,
      vec3(1.0, 1.0, 0.0) * 0.1f,
    };

  vec3 finalColor = getAmbientColor(objectColor, normal, metallicRoughness) + Lo;

  fragColor = vec4(finalColor, 1.0);
}
