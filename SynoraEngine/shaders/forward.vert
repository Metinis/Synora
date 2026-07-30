layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec4 aTangent;
layout(location = 4) in ivec4 aBoneID;
layout(location = 5) in vec4 aBoneWeight;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;

out vec3 fragNormal;
out vec3 fragPos;
out vec2 fragTexCoords;

#ifdef FEATURE_NORMAL
out mat3 TBN;
#endif

void main() {
  #ifdef FEATURE_NORMAL
  vec3 T = normalize(mat3(u_Model) * aTangent.xyz);
  vec3 N = normalize(mat3(u_Model) * aNormal);
  T = normalize(T - N * dot(N, T));

  vec3 B = cross(N, T) * aTangent.w;

  TBN = mat3(T, B, N);
  #endif

  fragNormal = normalize(mat3(transpose(inverse(u_Model))) * aNormal);
  fragPos = vec3(u_Model * vec4(aPos, 1.0));
  fragTexCoords = aTexCoords;

  gl_Position = u_ViewProjection * u_Model * vec4(aPos, 1.0);
}
