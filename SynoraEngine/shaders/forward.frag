out vec4 fragColor;

in vec3 fragNormal;
in vec3 fragPos;
in vec2 fragTexCoords;

uniform vec3 u_tint;
uniform float u_roughness;
uniform float u_metallic;

uniform sampler2D u_albedoTexture;

#ifdef FEATURE_METALLIC_ROUGHNESS
uniform sampler2D u_metallicRoughness;
#endif

uniform vec3 u_cameraPos;

const float PI = 3.14159265359;
const float MAX_REFLECTION_LOD = 4.0;

struct DirectionalLight {
  vec3 direction;
  vec3 color;
  float intensity;
  bool castShadow;
};

uniform DirectionalLight u_light;

#ifdef FEATURE_NORMAL
uniform sampler2D u_normalMap;
in mat3 TBN;
#endif

uniform samplerCube u_irradianceMap;
uniform samplerCube u_prefilterMap;
uniform sampler2D u_brdfLUT;

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

vec3 getNormal() {
  #ifdef FEATURE_NORMAL
  vec3 mapNormal = texture(u_normalMap, fragTexCoords).xyz;
  mapNormal = normalize(TBN * (mapNormal * 2.f - 1.f));
  vec3 normal = mapNormal;
  #else
  vec3 normal = normalize(fragNormal);
  #endif

  return normal;
}

vec2 getMetallicRoughness() {
  float roughness = u_roughness;
  float metallic = u_metallic;

  #ifdef FEATURE_METALLIC_ROUGHNESS
  roughness *= texture(u_metallicRoughness, fragTexCoords).g;
  metallic *= texture(u_metallicRoughness, fragTexCoords).b;
  #endif

  roughness = clamp(roughness, 0.045, 1.0);

  return vec2(metallic, roughness);
}

vec3 applyDirectionalLight(DirectionalLight light, vec3 objectColor) {
  vec3 lightDir = -light.direction;
  vec3 viewDir = normalize(u_cameraPos - fragPos);
  vec3 halfwayDir = normalize(lightDir + viewDir);
  vec3 radiance = light.color * light.intensity;

  vec2 metallicRoughness = getMetallicRoughness();
  float metallic = metallicRoughness.x;
  float roughness = metallicRoughness.y;

  vec3 normal = getNormal();

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

  float NdotL = max(dot(normal, lightDir), 0.0);
  return (kD * objectColor / PI + specular) * radiance * NdotL;
}

vec3 getAmbientColor(vec3 objectColor) {
  vec3 normal = getNormal();
  vec3 viewDir = normalize(u_cameraPos - fragPos);

  vec2 metallicRoughness = getMetallicRoughness();
  float metallic = metallicRoughness.x;
  float roughness = metallicRoughness.y;

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, objectColor, metallic);
  vec3 kS = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);
  vec3 kD = vec3(1.0) - kS;
  kD *= (1.0 - metallic);

  vec3 irradiance = texture(u_irradianceMap, normal).rgb;
  vec3 diffuse = irradiance * objectColor;
  return kD * diffuse;
}

vec3 getSpecularColor(vec3 objectColor) {
  vec3 normal = getNormal();
  vec3 viewDir = normalize(u_cameraPos - fragPos);
  vec2 metallicRoughness = getMetallicRoughness();

  float metallic = metallicRoughness.x;
  float roughness = metallicRoughness.y;

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, objectColor, metallic);

  vec3 reflectDir = reflect(-viewDir, normal);
  vec3 prefilterColor = textureLod(u_prefilterMap, reflectDir, roughness * MAX_REFLECTION_LOD).rgb;
  vec3 F = fresnelSchlickRoughness(max(dot(normal, viewDir), 0.0), F0, roughness);
  vec2 envBRDF = texture(u_brdfLUT, vec2(max(dot(normal, viewDir), 0.0), roughness)).rg;
  vec3 specular = prefilterColor * (F * envBRDF.x + envBRDF.y);
  return specular;
}

void main() {
  vec4 albedoTexture = texture(u_albedoTexture, fragTexCoords);
  if (albedoTexture.a < 0.1) discard;
  vec3 objectColor = vec3(vec4(u_tint, 1.0) * albedoTexture);

  vec3 Lo = applyDirectionalLight(u_light, objectColor);

  vec3 finalColor = getAmbientColor(objectColor) + getSpecularColor(objectColor) + Lo;

  fragColor = vec4(finalColor, 1.0);
}
