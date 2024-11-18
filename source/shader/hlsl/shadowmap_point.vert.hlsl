/// File: shadowmap_point.vert.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_position main(vsinput_mesh input) {
    float4x4 world_matrix;
    if (c_hasAnimation == 0) {
        world_matrix = c_worldMatrix;
    } else {
        float4x4 bone_matrix = c_bones[input.boneIndex.x] * input.boneWeight.x;
        bone_matrix += c_bones[input.boneIndex.y] * input.boneWeight.y;
        bone_matrix += c_bones[input.boneIndex.z] * input.boneWeight.z;
        bone_matrix += c_bones[input.boneIndex.w] * input.boneWeight.w;
        world_matrix = mul(c_worldMatrix, bone_matrix);
    }

    float4 position = mul(world_matrix, float4(input.position, 1.0));
    vsoutput_position output;
    output.world_position = position.xyz;
    output.position = mul(c_pointLightMatrix, position);
    return output;
}
