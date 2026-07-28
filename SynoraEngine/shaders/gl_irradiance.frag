#version 450 core

out vec4 fragColor;
in vec3 fragTexCoords;

const float PI = 3.14159265359;
uniform samplerCube u_hdrMap;

void main() {
  vec3 normal = normalize(fragTexCoords);

  vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
  vec3 right = normalize(cross(up, normal));
  up = normalize(cross(normal, right));

  vec3 irradiance = vec3(0.0);

  float sampleDelta = 0.025;
  int nrSamples = 0;

  for (float phi = 0.0; phi < 2.0f * PI; phi += sampleDelta) {
    for (float theta = 0.0; theta < PI * 0.5f; theta += sampleDelta) {
      vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
      vec3 sampleVector = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

      irradiance += textureLod(u_hdrMap, sampleVector, 3.0).rgb * cos(theta) * sin(theta);
      ++nrSamples;
    }
  }

  irradiance = PI * irradiance * (1.0f / float(nrSamples));

  fragColor = vec4(irradiance, 1.0f);
}
