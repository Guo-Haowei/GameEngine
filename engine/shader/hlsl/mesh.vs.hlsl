/// File: mesh.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

VS_OUTPUT_MESH main(VS_INPUT_MESH input,
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

    float3 T = normalize(mul(world_matrix, float4(input.tangent, 0.0f))).xyz;
    float3 N = normalize(mul(world_matrix, float4(input.normal, 0.0f))).xyz;
    float3 B = cross(N, T);

    float4 position = float4(input.position, 1.0);
    position = mul(world_matrix, position);
    position = mul(c_viewMatrix, position);
    float3 view_position = position.xyz;
    position = mul(c_projectionMatrix, position);

    VS_OUTPUT_MESH result;
    result.position = position;
    result.world_position = view_position;
    // @TODO: fix normal
    result.normal = N;
    result.T = T;
    result.B = B;
    result.uv = input.uv;
    return result;
}
