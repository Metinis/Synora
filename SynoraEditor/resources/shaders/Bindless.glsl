#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

#define LINEAR_SAMPLER_INDEX 0
#define NEAREST_SAMPLER_INDEX 1

layout(set = 0, binding = 0) uniform texture2D textures[];
layout(set = 0, binding = 0) uniform textureCube cubeMaps[];
layout(set = 0, binding = 1) uniform sampler samplers[];

vec4 sample2DLinear(uint textureIndex, vec2 uv) {
    return texture(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_INDEX)]), uv);
}
vec4 sample2DNearest(uint textureIndex, vec2 uv) {
    return texture(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(NEAREST_SAMPLER_INDEX)]), uv);
}

vec4 sampleCubeLinear(uint cubeMapIndex, vec3 dir) {
    return texture(samplerCube(cubeMaps[nonuniformEXT(cubeMapIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_INDEX)]), dir);
}
vec4 sampleCubeNearest(uint cubeMapIndex, vec3 dir) {
    return texture(samplerCube(cubeMaps[nonuniformEXT(cubeMapIndex)], samplers[nonuniformEXT(NEAREST_SAMPLER_INDEX)]), dir);
}
