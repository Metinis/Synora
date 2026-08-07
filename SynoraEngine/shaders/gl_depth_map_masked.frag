#version 450 core

layout(binding = 0) uniform sampler2D u_albedoTexture;

in vec2 fragTexCoords;

uniform float u_alphaCutoff;

void main() {
  vec4 albedoTexture = texture(u_albedoTexture, fragTexCoords);
  if (albedoTexture.a < u_alphaCutoff) discard;
}
