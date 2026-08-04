#version 450 core
#extension GL_ARB_shader_viewport_layer_array : require

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;

uniform mat4 u_modelMatrix;
uniform int u_layerOffset;

out vec2 fragTexCoords;

#define CASCADE_COUNT 4
layout(std140, binding = 1) uniform ShadowConstants {
  mat4 u_lightSpaceMatrices[CASCADE_COUNT];
  vec4 u_cascadePlaneDistances;
  vec4 u_cascadeTexelWorldSize;
};

void main() {
  fragTexCoords = aTexCoords;
  gl_Layer = gl_InstanceID;
  gl_Position = u_lightSpaceMatrices[gl_InstanceID + u_layerOffset] * u_modelMatrix * vec4(aPos, 1.0);
}
