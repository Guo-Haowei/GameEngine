/// File: voxelization_post.cs.hlsl
#include "shader_defines.hlsl.h"
#include "unordered_access_defines.hlsl.h"

[numthreads(COMPUTE_LOCAL_SIZE_VOXEL, COMPUTE_LOCAL_SIZE_VOXEL, COMPUTE_LOCAL_SIZE_VOXEL)] void main(uint3 p_dispatch_thread_id : SV_DISPATCHTHREADID) {
    int3 uvw = int3(p_dispatch_thread_id);
    float4 color = u_VoxelLighting[uvw];
    float4 normal = u_VoxelNormal[uvw];
    if (color.a < 0.000001) {
        return;
    }

    u_VoxelLighting[uvw] = color;
    u_VoxelNormal[uvw] = normal;
}
