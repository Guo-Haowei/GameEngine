#include "cbuffer.h"
#include "hlsl/input_output.hlsl"

float4 main(vsinput_position input, uint instance_id
            : SV_INSTANCEID) : SV_POSITION {
    float4 transform = globalParticleTransforms[instance_id];
    float tx = transform.x;
    float ty = transform.y;
    float tz = transform.z;
    float s = transform.w;

    float4x4 scale_matrix = float4x4(
        s, 0.0, 0.0, 0.0,
        0.0, s, 0.0, 0.0,
        0.0, 0.0, s, 0.0,
        0.0, 0.0, 0.0, 1.0);

    float4x4 model_matrix = float4x4(
        1.0, 0.0, 0.0, tx,
        0.0, 1.0, 0.0, ty,
        0.0, 0.0, 1.0, tz,
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
