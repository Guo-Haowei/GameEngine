/// File: prefilter.pixel.glsl
#include "../cbuffer.hlsl.h"
#include "lighting.glsl"

layout(location = 0) out vec4 out_color;
in vec3 pass_position;

#define SAMPLE_COUNT 1024u

void main() {
    vec3 N = normalize(pass_position);

    // make the simplyfying assumption that V equals R equals the normal
    vec3 R = N;
    vec3 V = R;

    float roughness = c_envPassRoughness;

    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = reflect(-V, H);

        float NdotL = dot(N, L);
        if (NdotL > 0.0) {
            // sample from the environment's mip level based on roughness/pdf
            float NdotH = max(dot(N, H), 0.0);
            float D = distribution_ggx(NdotH, roughness);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            // @TODO: fix hard code
            float resolution = 512.0;  // resolution of source cubemap (per face)
            float saTexel = 4.0 * MY_PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            prefilteredColor += textureLod(c_envMap, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;
    out_color = vec4(prefilteredColor, 1.0);
}