#ifdef VERTEX_SRC
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;

#include "common.glsl"

uniform mat4 u_Model;

out vec2 fragTexCoords;

invariant gl_Position;

void main() {
  fragTexCoords = aTexCoords;
  gl_Position = u_ViewProjection * u_Model * vec4(aPos, 1.0);
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
