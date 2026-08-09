#ifdef VERTEX_SRC
layout(location = 0) in vec3 aPos;

uniform mat4 u_ViewProjection;

out vec3 fragTexCoords;

void main() {
  fragTexCoords = aPos;
  vec4 vertexPos = u_ViewProjection * vec4(aPos, 1.0);
  gl_Position = vertexPos.xyww;
}
#endif
#ifdef FRAGMENT_SRC
out vec4 fragColor;
in vec3 fragTexCoords;

uniform samplerCube u_hdrMap;
uniform float u_roughness;

const float PI = 3.14159265359;

#include "importance_sample.glsl"
#include "pbr_brdf.glsl"

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
      float D = DistributionGGX(normal, halfwayDir, u_roughness);
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
#endif
