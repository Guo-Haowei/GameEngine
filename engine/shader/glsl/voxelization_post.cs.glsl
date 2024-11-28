/// File: voxelization_post.cs.glsl
#include "../shader_defines.hlsl.h"

layout(local_size_x = COMPUTE_LOCAL_SIZE_VOXEL, local_size_y = COMPUTE_LOCAL_SIZE_VOXEL, local_size_z = COMPUTE_LOCAL_SIZE_VOXEL) in;
layout(rgba16f, binding = 0) uniform image3D u_albedo_texture;
layout(rgba16f, binding = 1) uniform image3D u_normal_texture;

void main() {
    ivec3 uvw = ivec3(gl_GlobalInvocationID);
    vec4 color = imageLoad(u_albedo_texture, uvw);
    vec4 normal = imageLoad(u_normal_texture, uvw);
    if (color.a < 0.0001)
        return;

    color /= color.a;
    normal /= normal.a;
    imageStore(u_albedo_texture, uvw, color);
    imageStore(u_normal_texture, uvw, normal);
}
