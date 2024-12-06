/// File: skybox.ps.glsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

#if 0
void main() {
    vec3 color = texture(c_envMap, pass_position).rgb;
    // vec3 color = texture(c_diffuseIrradianceMap, pass_position).rgb;
    // vec3 color = textureLod(c_prefilteredMap, pass_position, 1.0).rgb;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    out_color = vec4(color, 1.0);
    // out_color = vec4(1.0);
}
#endif

float2 sample_spherical_map(float3 v) {
    const float2 inv_atan = float2(0.1591, 0.3183);
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

float4 main(vsoutput_position input) : SV_TARGET {
    float2 uv = sample_spherical_map(normalize(input.world_position));
    float3 color = TEXTURE_2D(EnvMapHdr).Sample(s_linearClampSampler, uv).rgb;
    return float4(color, 1.0f);
}
