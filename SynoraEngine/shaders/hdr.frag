#version 450 core

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
