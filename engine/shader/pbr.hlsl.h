/// File: pbr.hlsl.h
#ifndef PBR_HLSL_H_INCLUDED
#define PBR_HLSL_H_INCLUDED
#include "shader_defines.hlsl.h"

#if defined(__cplusplus)
#include <engine/math/vector.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
using namespace my;
using ::my::math::cross;
using ::my::math::max;
using ::my::math::min;
using ::my::math::normalize;
#endif

#define MAX_REFLECTION_LOD 4.0

// @TODO: refactor the functions
// NDF(n, h, alpha) = alpha^2 / (pi * ((n dot h)^2 * (alpha^2 - 1) + 1)^2)
float DistributionGGX(float NdotH, float p_roughness) {
    float a = p_roughness * p_roughness;
    float a2 = a * a;
    float nom = a2;
    float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
    denom = MY_PI * denom * denom;
    // if roughness = 0, NDF = 0,
    // if roughness = 1, NDF = 1 / pi
    return nom / denom;
}

float GeometrySchlickGGX(float p_n_dot_h, float p_roughness) {
    // note that we use a different k for IBL
    float a = p_roughness;
    float k = (a * a) / 2.0f;

    float nom = p_n_dot_h;
    float denom = p_n_dot_h * (1.0f - k) + k;

    return nom / denom;
}

// GSchlickGGX(n, v, k) = dot(n, v) / (dot(n, v)(1 - k) + k)
// k is a remapping of alpha
float GSchlickGGX(float p_n_dot_h, float p_roughness) {
    float r = (p_roughness + 1.0f);
    float k = (r * r) / 8.0f;

    float nom = p_n_dot_h;
    float denom = p_n_dot_h * (1.0f - k) + k;
    return nom / denom;
}

float GeometrySmith(float p_n_dot_v, float p_n_dot_l, float p_roughness) {
    float ggx2 = GeometrySchlickGGX(p_n_dot_v, p_roughness);
    float ggx1 = GeometrySchlickGGX(p_n_dot_l, p_roughness);

    return ggx1 * ggx2;
}

Vector3f FresnelSchlick(float p_cos_theta, const Vector3f p_f0) {
    return p_f0 + (1.0f - p_f0) * pow(1.0f - p_cos_theta, 5.0f);
}

float RadicalInverseVDC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;  // / 0x100000000
}

Vector2f Hammersley(uint i, uint N) {
    return Vector2f(float(i) / float(N), RadicalInverseVDC(i));
}

Vector3f ImportanceSampleGGX(Vector2f Xi, Vector3f N, float roughness) {
    float a = roughness * roughness;

    float phi = 2.0f * MY_PI * Xi.x;
    float cos_theta = sqrt((1.0f - Xi.y) / (1.0f + (a * a - 1.0f) * Xi.y));
    float sin_theta = sqrt(1.0f - cos_theta * cos_theta);

    // from spherical coordinates to cartesian coordinates - halfway vector
    Vector3f H;
    H.x = cos(phi) * sin_theta;
    H.y = sin(phi) * sin_theta;
    H.z = cos_theta;

    // from tangent-space H vector to world-space sample vector
    Vector3f up = abs(N.z) < 0.999f ? Vector3f(0.0f, 0.0f, 1.0f) : Vector3f(1.0f, 0.0f, 0.0f);
    Vector3f tangent = normalize(cross(up, N));
    Vector3f bitangent = cross(N, tangent);

    Vector3f sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

Vector3f FresnelSchlickRoughness(float p_cos_theta, Vector3f F0, float p_roughness) {
    Vector3f zero = Vector3f(0.0f, 0.0f, 0.0f);
    Vector3f tmp = Vector3f(1.0f, 1.0f, 1.0f) - p_roughness;
    return F0 + (max(tmp - F0, zero)) * pow(1.0f - p_cos_theta, 5.0f);
}

Vector2f IntegrateBRDF(float NdotV, float roughness) {
    Vector3f V;
    V.x = sqrt(1.0f - NdotV * NdotV);
    V.y = 0.0f;
    V.z = NdotV;

    float A = 0.0f;
    float B = 0.0f;

    Vector3f N = Vector3f(0.0f, 0.0f, 1.0f);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
        Vector2f Xi = Hammersley(i, SAMPLE_COUNT);
        Vector3f H = ImportanceSampleGGX(Xi, N, roughness);
        Vector3f L = normalize(2.0f * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0f);
        float NdotH = max(H.z, 0.0f);
        float VdotH = max(dot(V, H), 0.0f);

        if (NdotL > 0.0f) {
            float G = GeometrySmith(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0f - VdotH, 5.0f);

            A += (1.0f - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return Vector2f(A, B);
}

#endif