const float PI = 3.141592653589793;

float ggxNDF(float roughness, float nDotH) {
    float a2 = roughness * roughness;

    float denom = 1.f + (nDotH * nDotH) * (a2 - 1.f);
    denom = PI * denom * denom;
    return a2 / denom;
}

// height correlated Smith G2 for ggx + normalization factor for brdf
// equal to ggx
float ggxVisibility(float roughness, float nDotL, float nDotV) {
    if (nDotL <= 0 || nDotV <= 0) {
        return 0.f;
    }

    float uo = nDotV;
    float ui = nDotL;

    float a2 = roughness * roughness;

    float ui2 = ui * ui;
    float uo2 = uo * uo;

    float denom = uo * sqrt(a2 + ui2 * (1.f - a2));
    denom += ui * sqrt(a2 + uo2 * (1.f - a2));

    return 0.5f / denom;
}

float ggxDelta(float roughness, float s) {
    float a = s / (roughness * sqrt(1.f - s * s));
    float res = -1.f + sqrt(1.f + 1.f / (a * a));
    res /= 2.f;

    return res;
}

// height correlated smith masking function()
float ggxG2(float roughness, float nDotL, float nDotV) {
    return 1.f / (1.f + ggxDelta(roughness, nDotL) + ggxDelta(roughness, nDotV));
}
