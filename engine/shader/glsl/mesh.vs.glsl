/// File: mesh.vs.glsl
#include "../cbuffer.hlsl.h"
#include "../vsinput.glsl.h"

out struct PS_INPUT {
    vec3 position;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
} ps_in;

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

    // view space position
    vec4 position = c_viewMatrix * (world_matrix * vec4(in_position, 1.0));

    vec3 T = normalize(world_matrix * vec4(in_tangent, 0.0)).xyz;
    vec3 N = normalize(world_matrix * vec4(in_normal, 0.0)).xyz;
    vec3 B = cross(N, T);

    gl_Position = c_projectionMatrix * position;

    ps_in.position = position.xyz;
    ps_in.uv = in_uv;
    ps_in.T = T;
    ps_in.B = B;
    ps_in.N = N;
}
