#ifndef DESCRIPTOR_TABLE_DEFINES_HLSL_H_INCLUDED
#define DESCRIPTOR_TABLE_DEFINES_HLSL_H_INCLUDED

#define DESCRIPTOR_SRV_LIST                              \
    DESCRIPTOR_SRV(Texture2D, 128, _RESOURCE_DUMMY, 0)   \
    DESCRIPTOR_SRV(Texture2DArray, 16, Texture2D, 1)     \
    DESCRIPTOR_SRV(TextureCube, 16, Texture2DArray, 2)   \
    DESCRIPTOR_SRV(TextureCubeArray, 16, TextureCube, 3) \
    DESCRIPTOR_SRV(Texture3D, 8, TextureCubeArray, 4)
#define DESCRIPTOR_UAV_LIST                               \
    DESCRIPTOR_UAV(RWTexture2D, 16, Texture3D, 5, float3) \
    DESCRIPTOR_UAV(RWTexture3D, 8, RWTexture2D, 6, float4)

#define DESCRIPTOR_TABLE DESCRIPTOR_SRV_LIST DESCRIPTOR_UAV_LIST

#endif
