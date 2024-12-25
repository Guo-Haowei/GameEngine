/// File: voxelization.vs.glsl
#include "../cbuffer.hlsl.h"
#include "../vsinput.glsl.h"

out vec3 pass_positions;
out vec3 pass_normals;
out vec2 pass_uvs;

void main() {
    mat4 world_matrix;
    switch (c_meshFlag) {
        case MESH_HAS_BONE: {
            mat4 bone_matrix = c_bones[in_bone_id.x] * in_bone_weight.x;
            bone_matrix += c_bones[in_bone_id.y] * in_bone_weight.y;
            bone_matrix += c_bones[in_bone_id.z] * in_bone_weight.z;
            bone_matrix += c_bones[in_bone_id.w] * in_bone_weight.w;
            world_matrix = c_worldMatrix * bone_matrix;
        } break;
        case MESH_HAS_INSTANCE: {
            world_matrix = c_bones[gl_InstanceID];
        } break;
        default: {
            world_matrix = c_worldMatrix;
        } break;
    }

    vec4 world_position = world_matrix * vec4(in_position, 1.0);
    pass_positions = world_position.xyz;
    vec3 normal = normalize(in_normal);
    pass_normals = normalize((world_matrix * vec4(normal, 0.0)).xyz);
    pass_uvs = in_uv;
    gl_Position = world_position;
}
