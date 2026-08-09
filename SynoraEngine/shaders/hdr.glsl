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
uniform sampler2D u_hdrBuffer;

uniform float u_exposure;
uniform float u_gamma;

out vec4 fragColor;

void main() {
  vec3 hdrColor = texture(u_hdrBuffer, fragTexCoords).rgb;

  vec3 mapped = vec3(1.0) - exp(-hdrColor * u_exposure);
  mapped = pow(mapped, vec3(1.0 / u_gamma));

  fragColor = vec4(mapped, 1.0);
}
#endif
