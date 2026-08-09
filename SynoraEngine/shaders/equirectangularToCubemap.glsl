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

uniform sampler2D u_equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 sampleSphericalMap(vec3 v) {
  vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
  uv *= invAtan;
  uv += 0.5;
  return uv;
}

void main() {
  vec2 uv = sampleSphericalMap(normalize(fragTexCoords));
  fragColor = texture(u_equirectangularMap, uv);
}
#endif
