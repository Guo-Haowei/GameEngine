/// File: particle_draw.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

#if defined(HLSL_LANG_D3D11)
#include "structured_buffer.hlsl.h"
#define SBUFFER(DATA_TYPE, NAME, REG, REG2) \
    StructuredBuffer<DATA_TYPE> NAME : register(t##REG);
SBUFFER_LIST
#undef SBUFFER
#else
#include "shader_resource_defines.hlsl.h"
#endif

VS_OUTPUT_UV main(VS_INPUT_POS input, uint instance_id : SV_INSTANCEID) {
    VS_OUTPUT_UV output;

    Particle particle = GlobalParticleData[instance_id];
    if (particle.lifeRemaining <= 0.0) {
        output.uv = float2(0.0, 0.0);
        output.position = float4(1000.0, 1000.0, 1000.0, 1.0);
        return output;
    }

    float3 t = particle.position.xyz;
    float s = particle.scale;

    float4x4 scale_matrix = float4x4(
        s, 0.0, 0.0, 0.0,
        0.0, s, 0.0, 0.0,
        0.0, 0.0, s, 0.0,
        0.0, 0.0, 0.0, 1.0);

    float4x4 model_matrix = float4x4(
        1.0, 0.0, 0.0, t.x,
        0.0, 1.0, 0.0, t.y,
        0.0, 0.0, 1.0, t.z,
        0.0, 0.0, 0.0, 1.0);

    // make sure always face to screen
    // https://blog.42yeah.is/opengl/rendering/2023/06/24/opengl-billboards.html
    float4x4 view_model_matrix = mul(c_viewMatrix, model_matrix);
    view_model_matrix[0][0] = 1.0;
    view_model_matrix[0][1] = 0.0;
    view_model_matrix[0][2] = 0.0;
    view_model_matrix[1][0] = 0.0;
    view_model_matrix[1][1] = 1.0;
    view_model_matrix[1][2] = 0.0;
    view_model_matrix[2][0] = 0.0;
    view_model_matrix[2][1] = 0.0;
    view_model_matrix[2][2] = 1.0;

    float4 position = mul(scale_matrix, float4(input.position, 1.0));
    position = mul(view_model_matrix, position);
    position = mul(c_projectionMatrix, position);

    output.position = position;
    float2 uv = 0.5f * (input.position.xy + 1.0f);

    // @TODO FIX THIS HACK!!!!
    {
        int counter = (c_subUvCounter / 4) % 16;
        int u = counter % 4;
        int v = counter / 4;
        uv *= 0.25;
        uv += 0.25 * float2(u, v);
    }

    output.uv = uv;
    return output;
}
