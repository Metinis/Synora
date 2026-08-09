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
