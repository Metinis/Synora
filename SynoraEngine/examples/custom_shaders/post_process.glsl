#ifdef VERTEX_SRC
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 fragTexCoords;

void main() {
  fragTexCoords = aTexCoords;
  gl_Position = vec4(aPos, 0.0, 1.0);
}
#endif
#ifdef FRAGMENT_SRC
in vec2 fragTexCoords;
layout(binding = 0) uniform sampler2D u_Buffer;

uniform float u_Time;

out vec4 fragColor;

void main() {
  vec2 uv = fragTexCoords;

  float mult = cos(u_Time) + 1.5;

  vec3 bufferColor = texture(u_Buffer, uv).rgb * mult;

  fragColor = vec4(bufferColor, 1.0);
}
#endif
