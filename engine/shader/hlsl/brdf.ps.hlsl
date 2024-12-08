/// File: brdf.ps.hlsl
#include "hlsl/input_output.hlsl"
#include "pbr.hlsl"
#include "shader_defines.hlsl.h"

Vector2f IntegrateBRDF(float NdotV, float roughness) {
    Vector3f V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    Vector3f N = Vector3f(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        // generates a sample vector that's biased towards the
        // preferred alignment direction (importance sampling).
        Vector2f Xi = Hammersley(i, SAMPLE_COUNT);
        Vector3f H = ImportanceSampleGGX(Xi, N, roughness);
        Vector3f L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0) {
            float G = geometry_smith(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return Vector2f(A, B);
}

float2 main(vsoutput_uv input) : SV_TARGET {
    Vector2f integratedBRDF = IntegrateBRDF(input.uv.x, input.uv.y);
    return integratedBRDF;
}
