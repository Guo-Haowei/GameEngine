/// File: shadowmap_point.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

VS_OUTPUT_POSITION main(VS_INPUT_MESH input,
                        uint instance_id : SV_InstanceID) {
    float4x4 world_matrix;
    switch (c_meshFlag) {
        case MESH_HAS_BONE: {
            float4x4 bone_matrix = c_bones[input.boneIndex.x] * input.boneWeight.x;
            bone_matrix += c_bones[input.boneIndex.y] * input.boneWeight.y;
            bone_matrix += c_bones[input.boneIndex.z] * input.boneWeight.z;
            bone_matrix += c_bones[input.boneIndex.w] * input.boneWeight.w;
            world_matrix = mul(c_worldMatrix, bone_matrix);
        } break;
        case MESH_HAS_INSTANCE: {
            world_matrix = c_bones[instance_id];
        } break;
        default: {
            world_matrix = c_worldMatrix;
        } break;
    }

    float4 position = mul(world_matrix, float4(input.position, 1.0));
    VS_OUTPUT_POSITION output;
    output.world_position = position.xyz;
    output.position = mul(c_pointLightMatrix, position);
    return output;
}
