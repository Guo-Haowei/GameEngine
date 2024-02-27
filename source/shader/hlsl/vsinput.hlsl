// struct vsinput_pos_color
// {
//     float3 position : POSITION;
//     float3 color : COLOR;
// };

// struct vsinput_pos2_uv
// {
//     float2 position : POSITION;
//     float2 uv : TEXCOORD;
// };

// struct vsinput_pos_uv_norm
// {
//     float3 position : POSITION;
//     float2 uv : TEXCOORD;
//     float3 normal : NORMAL;
// };

struct vsinput_mesh {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    int4 boneIndex : BONEINDEX;
    float4 boneWeight : BONEWEIGHT;
};