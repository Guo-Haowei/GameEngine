/// File: particle_draw.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "structured_buffer.hlsl.h"
// #include "shader_resource_defines.hlsl.h"

// @TODO: refactor
StructuredBuffer<Particle> GlobalParticleData : register(t24);

vsoutput_color main(vsinput_position input, uint instance_id : SV_INSTANCEID) {
    vsoutput_color output;

    Particle particle = GlobalParticleData[instance_id];
    if (particle.lifeRemaining <= 0.0) {
        output.color = float4(1.0, 1.0, 1.0, 1.0);
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
    output.color.rgb = particle.color.rgb;
    return output;
}
