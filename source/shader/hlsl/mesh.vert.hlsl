/// File: mesh.vert.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_mesh main(vsinput_mesh input) {
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

    float3 T = normalize(mul(world_matrix, float4(input.tangent, 0.0f))).xyz;
    float3 N = normalize(mul(world_matrix, float4(input.normal, 0.0f))).xyz;
    float3 B = cross(N, T);

    float4 position = float4(input.position, 1.0);
    position = mul(world_matrix, position);
    float3 world_position = position.xyz;
    position = mul(c_viewMatrix, position);
    position = mul(c_projectionMatrix, position);

    vsoutput_mesh result;
    result.position = position;
    result.world_position = world_position;
    // @TODO: fix normal
    result.normal = N;
    result.T = T;
    result.B = B;
    result.uv = input.uv;
    return result;
}
