/// File: tone.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

float main(vsoutput_uv input) : SV_TARGET {
    const Vector2f uv = input.uv;

    Vector3f N = TEXTURE_2D(GbufferNormalMap).Sample(s_linearClampSampler, uv).rgb;
    N = 2.0f * N - 1.0f;

    float D = TEXTURE_2D(GbufferDepth).Sample(s_linearClampSampler, uv).r;

    float r = D;
    return r;
}
