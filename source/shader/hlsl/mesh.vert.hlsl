/// File: mesh.vert.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_mesh main(vsinput_mesh input) {
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
    float3 world_position = position.xyz;
    position = mul(c_viewMatrix, position);
    position = mul(c_projectionMatrix, position);

    vsoutput_mesh result;
    result.position = position;
    result.world_position = world_position;
    // @TODO: fix normal
    float4 normal4 = mul(world_matrix, float4(input.normal, 0.0));
    result.normal = normal4.xyz;
    result.uv = input.uv;
    return result;
}
