#ifdef VERTEX_SRC
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;
layout(location = 4) in ivec4 aBoneID;
layout(location = 5) in vec4 aBoneWeight;

#include "common.glsl"

uniform mat4 u_Model;

#ifdef FEATURE_SKINNED
uniform mat4 boneTransforms[MAX_BONES];
#endif

out vec2 fragTexCoords;

invariant gl_Position;

void main() {
  vec4 vertexPos = vec4(aPos, 1.0);

  #ifdef FEATURE_SKINNED
  vertexPos = applyBoneTransform(boneTransforms, aBoneID, aBoneWeight, vertexPos);
  #endif

  fragTexCoords = aTexCoords;
  gl_Position = u_ViewProjection * u_Model * vertexPos;
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
