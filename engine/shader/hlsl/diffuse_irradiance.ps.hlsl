/// File: diffuse_irradiance.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

static const float sampleStep = 0.025;

float4 main(vsoutput_position input) : SV_TARGET {
    float3 N = normalize(input.world_position);
    float3 up = float3(0.0, 1.0, 0.0);
    float3 right = cross(up, N);
    up = cross(N, right);

    float3 irradiance = float3(0.0, 0.0, 0.0);
    float samples = 0.0;

    for (float phi = 0.0; phi < 2.0 * MY_PI; phi += sampleStep) {
        for (float theta = 0.0; theta < 0.5 * MY_PI; theta += sampleStep) {
            float xdir = sin(theta) * cos(phi);
            float ydir = sin(theta) * sin(phi);
            float zdir = cos(theta);
            float3 sampleVec = xdir * right + ydir * up + zdir * N;
            irradiance += TEXTURE_CUBE(Skybox).SampleLevel(s_cubemapClampSampler, sampleVec, 0.0).rgb * cos(theta) * sin(theta);
            samples += 1.0;
        }
    }

    irradiance = MY_PI * irradiance * (1.0 / samples);
    return float4(irradiance, 1.0);
}