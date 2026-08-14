#ifdef VERTEX_SRC

#ifdef FEATURE_DEPTH_MAP_INSTANCED
#extension GL_ARB_shader_viewport_layer_array : require
#endif

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;
layout(location = 4) in ivec4 aBoneID;
layout(location = 5) in vec4 aBoneWeight;

#include "common.glsl"

#ifdef FEATURE_DEPTH_MAP_INSTANCED

uniform int u_layerOffset;

#else
uniform mat4 u_lightSpaceMatrix;
#endif

uniform mat4 u_modelMatrix;

out vec2 fragTexCoords;

#ifdef FEATURE_SKINNED
uniform mat4 boneTransforms[MAX_BONES];
#endif

void main() {
  vec4 vertexPos = vec4(aPos, 1.0);

  #ifdef FEATURE_SKINNED
  vertexPos = applyBoneTransform(boneTransforms, aBoneID, aBoneWeight, vertexPos);
  #endif

  fragTexCoords = aTexCoords;
  #ifdef FEATURE_DEPTH_MAP_INSTANCED
  gl_Layer = gl_InstanceID;
  gl_Position = u_lightSpaceMatrices[gl_InstanceID + u_layerOffset] * u_modelMatrix * vertexPos;
  #else
  gl_Position = u_lightSpaceMatrix * u_modelMatrix * vertexPos;
  #endif
}
#endif
#ifdef FRAGMENT_SRC

#ifdef FEATURE_ALPHA_TEST
layout(binding = 0) uniform sampler2D u_albedoTexture;
in vec2 fragTexCoords;
uniform float u_alphaCutoff;
#endif

void main() {
  #ifdef FEATURE_ALPHA_TEST
  vec4 albedoTexture = texture(u_albedoTexture, fragTexCoords);
  if (albedoTexture.a < u_alphaCutoff) discard;
  #endif
}
#endif
