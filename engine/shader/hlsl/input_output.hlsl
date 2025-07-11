/// File: input_output.hlsl
// Input
struct VS_INPUT_POS {
    float3 position : POSITION;
};

struct VS_INPUT_SPRITE {
    float2 position : POSITION;
    float2 uv : TEXCOORD;
};

struct VS_INPUT_MESH {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    int4 boneIndex : BONEINDEX;
    float4 boneWeight : BONEWEIGHT;
    float4 color : COLOR;
};

// Output
struct VS_OUTPUT_UV {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct VS_OUTPUT_POSITION {
    float4 position : SV_POSITION;
    float3 world_position : POSITION;
};

struct VS_INPUT_COLOR {
    float3 position : POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT_COLOR {
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

struct VS_OUTPUT_MESH {
    float4 position : SV_POSITION;
    float3 world_position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 T : TANGENT;
    float3 B : BITANGENT;
};
