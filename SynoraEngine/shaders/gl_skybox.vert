#version 450 core
layout(location = 0) in vec3 aPos;

uniform mat4 u_ViewProjection;

out vec3 fragTexCoords;

void main() {
  fragTexCoords = aPos;
  vec4 vertexPos = u_ViewProjection * vec4(aPos, 1.0);
  gl_Position = vertexPos.xyww;
}
