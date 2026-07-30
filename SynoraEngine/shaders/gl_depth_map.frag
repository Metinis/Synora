#version 450 core
uniform sampler2D u_albedoTexture;

in vec2 fragTexCoords;

void main() {
  vec4 albedoTexture = texture(u_albedoTexture, fragTexCoords);
  if (albedoTexture.a < 0.1) discard;
}
