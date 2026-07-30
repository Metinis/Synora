#version 450 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;

uniform mat4 u_modelMatrix;
uniform mat4 u_lightSpaceMatrix;

out vec2 fragTexCoords;

void main() {
  fragTexCoords = aTexCoords;
  gl_Position = u_lightSpaceMatrix * u_modelMatrix * vec4(aPos, 1.0);
}
