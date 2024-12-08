/// File: pbr.hlsl
#ifndef PBR_HLSL_INCLUDED
#define PBR_HLSL_INCLUDED
#include "shader_defines.hlsl.h"

// @TODO: refactor the functions
// NDF(n, h, alpha) = alpha^2 / (pi * ((n dot h)^2 * (alpha^2 - 1) + 1)^2)
float distribution_ggx(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float nom = a2;
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    denom = MY_PI * denom * denom;
    // if roughness = 0, NDF = 0,
    // if roughness = 1, NDF = 1 / pi
    return nom / denom;
}

float geometry_schlick_ggx(float NdotV, float roughness) {
    // note that we use a different k for IBL
    float a = roughness;
    float k = (a * a) / 2.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

// g_schlick_ggx(n, v, k) = dot(n, v) / (dot(n, v)(1 - k) + k)
// k is a remapping of alpha
float g_schlick_ggx(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    return nom / denom;
}

float geometry_smith(float NdotV, float NdotL, float roughness) {
    float ggx2 = geometry_schlick_ggx(NdotV, roughness);
    float ggx1 = geometry_schlick_ggx(NdotL, roughness);

    return ggx1 * ggx2;
}

Vector3f fresnel_schlick(float cosTheta, const in Vector3f F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;  // / 0x100000000
}

Vector2f Hammersley(uint i, uint N) {
    return Vector2f(float(i) / float(N), RadicalInverse_VdC(i));
}

Vector3f ImportanceSampleGGX(Vector2f Xi, Vector3f N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0 * MY_PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates - halfway vector
    Vector3f H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space H vector to world-space sample vector
    Vector3f up = abs(N.z) < 0.999 ? Vector3f(0.0, 0.0, 1.0) : Vector3f(1.0, 0.0, 0.0);
    Vector3f tangent = normalize(cross(up, N));
    Vector3f bitangent = cross(N, tangent);

    Vector3f sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

////////////////////////


#endif