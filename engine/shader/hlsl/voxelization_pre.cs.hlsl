/// File: voxelization_pre.cs.hlsl
#include "shader_defines.hlsl.h"
#include "unordered_access_defines.hlsl.h"

[numthreads(COMPUTE_LOCAL_SIZE_VOXEL, COMPUTE_LOCAL_SIZE_VOXEL, COMPUTE_LOCAL_SIZE_VOXEL)] void main(uint3 p_dispatch_thread_id : SV_DISPATCHTHREADID) {
    int3 uvw = int3(p_dispatch_thread_id);

    float4 clear_color = float4(0.0, 0.0, 0.0, 0.0);

    u_VoxelLighting[uvw] = clear_color;
    u_VoxelNormal[uvw] = clear_color;
}
