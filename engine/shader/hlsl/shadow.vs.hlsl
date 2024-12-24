/// File: shadow.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

float4 main(vsinput_mesh input) : SV_POSITION {
    float4x4 world_matrix;
    if (c_meshFlag == 0) {
        world_matrix = c_worldMatrix;
    } else {
        float4x4 bone_matrix = c_bones[input.boneIndex.x] * input.boneWeight.x;
        bone_matrix += c_bones[input.boneIndex.y] * input.boneWeight.y;
        bone_matrix += c_bones[input.boneIndex.z] * input.boneWeight.z;
        bone_matrix += c_bones[input.boneIndex.w] * input.boneWeight.w;
        world_matrix = mul(c_worldMatrix, bone_matrix);
    }

    float4 position = float4(input.position, 1.0);
    position = mul(world_matrix, position);
    position = mul(c_viewMatrix, position);
    position = mul(c_projectionMatrix, position);

    return position;
}
