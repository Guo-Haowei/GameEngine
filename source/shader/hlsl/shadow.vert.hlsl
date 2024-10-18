#include "cbuffer.h"
#include "hlsl/input_output.hlsl"

float4 main(vsinput_mesh input) : SV_POSITION {
#ifdef HAS_ANIMATION
    float4x4 boneTransform = u_bones[input.boneIndex.x] * input.boneWeight.x;
    boneTransform += u_bones[input.boneIndex.y] * input.boneWeight.y;
    boneTransform += u_bones[input.boneIndex.z] * input.boneWeight.z;
    boneTransform += u_bones[input.boneIndex.w] * input.boneWeight.w;
    float4x4 world_matrix = mul(u_world_matrix, boneTransform);
#else
    float4x4 world_matrix = u_world_matrix;
#endif

    float4 position = float4(input.position, 1.0);
    position = mul(world_matrix, position);
    position = mul(u_proj_view_matrix, position);

    return position;
}
