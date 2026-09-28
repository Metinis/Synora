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
layout(binding = 1) uniform usampler2D u_Stencil;

out vec4 fragColor;

const float outlineSize = 4.5;

void main() {
  vec2 canvasSize = textureSize(u_Stencil, 0);
  vec2 uv = gl_FragCoord.xy / canvasSize;
  vec2 texelSize = 1.0 / canvasSize;
  uint center = texture(u_Stencil, uv).r;

  if (center > 0) discard;

  int outlineInt = int(ceil(outlineSize));
  float out2 = outlineSize * outlineSize;

  int nearSelected = 0;
  for (int y = -outlineInt; y <= outlineInt; ++y) {
    for (int x = -outlineInt; x <= outlineInt; ++x) {
      if (x * x + y * y > out2) continue;
      vec2 offset = vec2(x, y) * texelSize;
      uint s = texture(
          u_Stencil,
          uv + offset
        ).r;

      if (s == 1) {
        nearSelected = 1;
        break;
      }
    }
  }

  float outline =
    (center == 0u && nearSelected == 1)
    ? 1.0 : 0.0;

  fragColor = mix(texture(u_Buffer, uv), vec4(1.0, 0.0, 0.0, 1.0), outline);
}
#endif
