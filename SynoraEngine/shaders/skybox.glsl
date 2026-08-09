#ifdef VERTEX_SRC
layout(location = 0) in vec3 aPos;

uniform mat4 u_ViewProjection;

out vec3 fragTexCoords;

void main() {
  fragTexCoords = aPos;
  vec4 vertexPos = u_ViewProjection * vec4(aPos, 1.0);
  gl_Position = vertexPos.xyww;
}
#endif
#ifdef FRAGMENT_SRC
out vec4 fragColor;

in vec3 fragTexCoords;
uniform samplerCube u_skybox;

void main() {
  fragColor = texture(u_skybox, fragTexCoords);
}
#endif
