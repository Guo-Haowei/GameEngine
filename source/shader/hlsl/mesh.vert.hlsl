#include "vsinput.hlsl"
#include "vsoutput.hlsl"
// #include "cbuffer.hlsl"

// @TODO: fix
cbuffer PerFrameConstants : register(b0) {
    float4x4 Proj;
    float4x4 View;
    float4x4 ProjView;
    float4x4 _dummy0;
    // float3 EyePos;
    // int NumOfLights;
    // Light Lights[2];
};

cbuffer PerBatchConstants : register(b1) {
    float4x4 Model;
    float4x4 _dummy1;
    float4x4 _dummy2;
    float4x4 _dummy3;
};

vsoutput_mesh main(vsinput_mesh input) {
    float4x4 worldMatrix = Model;

    float4 position = float4(input.position, 1.0);
    position = mul(worldMatrix, position);
    float3 world_position = position.xyz;
    position = mul(ProjView, position);

    vsoutput_mesh result;
    result.position = position;
    result.world_position = world_position;
    // @TODO: fix normal
    float4 normal4 = mul(worldMatrix, float4(input.normal, 0.0));
    result.normal = normal4.xyz;
    result.uv = input.uv;
    return result;
}
