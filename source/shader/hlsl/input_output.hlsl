// Input
struct vsinput_position {
    float3 position : POSITION;
};

struct vsinput_mesh {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    int4 boneIndex : BONEINDEX;
    float4 boneWeight : BONEWEIGHT;
};

// Output
struct vsoutput_uv {
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct vsoutput_position {
    float4 position : SV_POSITION;
    float3 world_position : POSITION;
};

struct vsoutput_mesh {
    float4 position : SV_POSITION;
    float3 world_position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
};
