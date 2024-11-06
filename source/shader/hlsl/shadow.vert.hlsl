/// File: shadow.vert.hlsl
#include "cbuffer.h"
#include "hlsl/input_output.hlsl"

float4 main(vsinput_mesh input) : SV_POSITION {
#ifdef HAS_ANIMATION
    float4x4 boneTransform = c_bones[input.boneIndex.x] * input.boneWeight.x;
    boneTransform += c_bones[input.boneIndex.y] * input.boneWeight.y;
    boneTransform += c_bones[input.boneIndex.z] * input.boneWeight.z;
    boneTransform += c_bones[input.boneIndex.w] * input.boneWeight.w;
    float4x4 world_matrix = mul(c_worldMatrix, boneTransform);
#else
    float4x4 world_matrix = c_worldMatrix;
#endif

    float4 position = float4(input.position, 1.0);
    position = mul(world_matrix, position);
    position = mul(c_viewMatrix, position);
    position = mul(c_projectionMatrix, position);

    return position;
}
