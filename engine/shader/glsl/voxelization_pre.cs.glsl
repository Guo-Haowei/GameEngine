/// File: voxelization_post.pre.glsl
#include "../shader_defines.hlsl.h"

layout(local_size_x = COMPUTE_LOCAL_SIZE_VOXEL, local_size_y = COMPUTE_LOCAL_SIZE_VOXEL, local_size_z = COMPUTE_LOCAL_SIZE_VOXEL) in;
layout(rgba16f, binding = 0) uniform image3D u_albedo_texture;
layout(rgba16f, binding = 1) uniform image3D u_normal_texture;

void main() {
    ivec3 uvw = ivec3(gl_GlobalInvocationID);

    vec4 clear_color = vec4(0.0, 0.0, 0.0, 0.0);
    imageStore(u_albedo_texture, uvw, clear_color);
    imageStore(u_normal_texture, uvw, clear_color);
}
