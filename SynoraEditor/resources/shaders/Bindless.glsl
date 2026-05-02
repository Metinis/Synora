#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require

#define LINEAR_SAMPLER_CLAMPED_INDEX 0
#define NEAREST_SAMPLER_CLAMPED_INDEX 1
#define LINEAR_SAMPLER_REPEAT_INDEX 2
#define NEAREST_SAMPLER_REPEAT_INDEX 3

layout(set = 0, binding = 0) uniform texture2D textures[];
layout(set = 0, binding = 0) uniform textureCube cubeMaps[];
layout(set = 0, binding = 1) uniform sampler samplers[];

layout(set = 0, binding = 2, rgba8) uniform image2D rgba8Images[];
layout(set = 0, binding = 2, rgba16f) uniform image2D rgba16fImages[];
layout(set = 0, binding = 2, rgba32f) uniform image2D rgba32fImages[];
layout(set = 0, binding = 2, r32f) uniform image2D r32fImages[];

// w for writeonly
layout(set = 0, binding = 2, rgba8) writeonly uniform image2D rgba8ImagesW[];
layout(set = 0, binding = 2, rgba16f) writeonly uniform image2D rgba16fImagesW[];
layout(set = 0, binding = 2, rgba32f) writeonly uniform image2D rgba32fImagesW[];
layout(set = 0, binding = 2, r32f) writeonly uniform image2D r32fImagesW[];

// default clamped, R for repeating

vec4 sampleLinear(uint textureIndex, vec2 uv) {
    return texture(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_CLAMPED_INDEX)]), uv);
}
vec4 sampleNearest(uint textureIndex, vec2 uv) {
    return texture(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(NEAREST_SAMPLER_CLAMPED_INDEX)]), uv);
}

vec4 sampleLinear(uint cubeMapIndex, vec3 dir) {
    return texture(samplerCube(cubeMaps[nonuniformEXT(cubeMapIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_CLAMPED_INDEX)]), dir);
}
vec4 sampleNearest(uint cubeMapIndex, vec3 dir) {
    return texture(samplerCube(cubeMaps[nonuniformEXT(cubeMapIndex)], samplers[nonuniformEXT(NEAREST_SAMPLER_CLAMPED_INDEX)]), dir);
}

vec4 sampleLinearR(uint textureIndex, vec2 uv) {
    return texture(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_REPEAT_INDEX)]), uv);
}
vec4 sampleNearestR(uint textureIndex, vec2 uv) {
    return texture(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(NEAREST_SAMPLER_REPEAT_INDEX)]), uv);
}

vec4 sampleLinearR(uint cubeMapIndex, vec3 dir) {
    return texture(samplerCube(cubeMaps[nonuniformEXT(cubeMapIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_REPEAT_INDEX)]), dir);
}
vec4 sampleNearestR(uint cubeMapIndex, vec3 dir) {
    return texture(samplerCube(cubeMaps[nonuniformEXT(cubeMapIndex)], samplers[nonuniformEXT(NEAREST_SAMPLER_REPEAT_INDEX)]), dir);
}

float queryLod(uint textureIndex, vec2 uv) {
    return textureQueryLod(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_CLAMPED_INDEX)]), uv).y;
}

float queryLod(uint cubeMapIndex, vec3 dir) {
    return textureQueryLod(samplerCube(cubeMaps[nonuniformEXT(cubeMapIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_CLAMPED_INDEX)]), dir).y;
}
float queryLodNearest(uint cubeMapIndex, vec3 dir) {
    return textureQueryLod(samplerCube(cubeMaps[nonuniformEXT(cubeMapIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_CLAMPED_INDEX)]), dir).y;
}

int queryLevels(uint textureIndex) {
    return textureQueryLevels(sampler2D(textures[nonuniformEXT(textureIndex)], samplers[nonuniformEXT(LINEAR_SAMPLER_CLAMPED_INDEX)]));
}
