#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;

layout(std140, binding = 0) uniform CameraConstants {
  mat4 u_ViewProjection;
  mat4 u_View;
  vec3 u_cameraPos;
};

uniform mat4 u_Model;

out vec2 fragTexCoords;

invariant gl_Position;

void main() {
  fragTexCoords = aTexCoords;
  gl_Position = u_ViewProjection * u_Model * vec4(aPos, 1.0);
}
