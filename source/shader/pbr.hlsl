#ifndef PBR_HLSL_INCLUDED
#define PBR_HLSL_INCLUDED
#include "shader_defines.h"

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

vec3 fresnel_schlick(float cosTheta, const in vec3 F0) { return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); }

#endif