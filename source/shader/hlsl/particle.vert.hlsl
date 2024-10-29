#include "cbuffer.h"
#include "hlsl/input_output.hlsl"

float4 main(vsinput_position input, uint instance_id
            : SV_INSTANCEID) : SV_POSITION {
    float4 transform = globalPatricleTransforms[instance_id];
    float tx = transform.x;
    float ty = transform.y;
    float tz = transform.z;
    float s = transform.w;

    // float4x4 world_matrix = float4x4(
    //     s, 0.0, 0.0, 0.0,
    //     0.0, s, 0.0, 0.0,
    //     0.0, 0.0, s, 0.0,
    //     tx, ty, tz, 1.0);
    float4x4 world_matrix = float4x4(
        s, 0.0, 0.0, tx,
        0.0, s, 0.0, ty,
        0.0, 0.0, s, tz,
        0.0, 0.0, 0.0, 1.0);

    float4 position = mul(world_matrix, float4(input.position, 1.0));
    position = mul(g_view_matrix, position);
    position = mul(g_projection_matrix, position);
    return position;
}
