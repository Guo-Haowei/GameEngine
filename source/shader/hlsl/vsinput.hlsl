struct vsinput_mesh {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 uv : TEXCOORD;
    float3 tangent : TANGENT;
    int4 boneIndex : BONEINDEX;
    float4 boneWeight : BONEWEIGHT;
};
