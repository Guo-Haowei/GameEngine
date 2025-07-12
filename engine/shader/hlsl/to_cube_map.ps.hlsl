/// File: to_cube_map.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

Texture2D t_SkyboxHdr : register(t0);

float2 sample_spherical_map(float3 v) {
    const float2 inv_atan = float2(0.1591, 0.3183);
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

float4 main(VS_OUTPUT_POSITION input)
    : SV_TARGET {
    float2 uv = sample_spherical_map(normalize(input.world_position));
    float3 color = t_SkyboxHdr.Sample(s_linearClampSampler, uv).rgb;
    return float4(color, 1.0f);
}
