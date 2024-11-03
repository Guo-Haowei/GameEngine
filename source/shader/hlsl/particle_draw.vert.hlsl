#include "cbuffer.h"
#include "hlsl/input_output.hlsl"

// @TODO: shader naming style
struct Particle {
    vec4 position;
    vec4 velocity;
    vec4 color;

    float scale;
    float lifeSpan;
    float lifeRemaining;
    int isActive;
};

// @TODO: refactor
StructuredBuffer<Particle> GlobalParticleData : register(t20);

float4 main(vsinput_position input, uint instance_id
            : SV_INSTANCEID) : SV_POSITION {
    Particle particle = GlobalParticleData[instance_id];
    if (particle.lifeRemaining <= 0.0) {
        return float4(1000.0, 1000.0, 1000.0, 1.0);
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
    float4x4 view_model_matrix = mul(g_view_matrix, model_matrix);
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
    position = mul(g_projection_matrix, position);
    return position;
}
