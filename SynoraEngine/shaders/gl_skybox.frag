#version 450 core

out vec4 fragColor;

in vec3 fragTexCoords;
uniform samplerCube u_skybox;

void main() {
  fragColor = texture(u_skybox, fragTexCoords);
}
