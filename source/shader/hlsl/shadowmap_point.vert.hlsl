/// File: shadowmap_point.vert.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_position main(vsinput_mesh input) {
#ifdef HAS_ANIMATION
    float4x4 boneTransform = c_bones[input.boneIndex.x] * input.boneWeight.x;
    boneTransform += c_bones[input.boneIndex.y] * input.boneWeight.y;
    boneTransform += c_bones[input.boneIndex.z] * input.boneWeight.z;
    boneTransform += c_bones[input.boneIndex.w] * input.boneWeight.w;
    float4x4 world_matrix = mul(c_worldMatrix, boneTransform);
#else
    float4x4 world_matrix = c_worldMatrix;
#endif

    float4 position = mul(world_matrix, float4(input.position, 1.0));
    vsoutput_position output;
    output.world_position = position.xyz;
    output.position = mul(c_pointLightMatrix, position);
    return output;
}
