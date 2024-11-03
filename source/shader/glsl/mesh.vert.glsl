#include "../cbuffer.h"
#include "../vsinput.glsl.h"

out struct PS_INPUT {
    vec3 position;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
} ps_in;

void main() {
#ifdef HAS_ANIMATION
    mat4 bone_matrix = u_bones[in_bone_id.x] * in_bone_weight.x;
    bone_matrix += u_bones[in_bone_id.y] * in_bone_weight.y;
    bone_matrix += u_bones[in_bone_id.z] * in_bone_weight.z;
    bone_matrix += u_bones[in_bone_id.w] * in_bone_weight.w;

    mat4 world_matrix = c_worldMatrix * bone_matrix;
#else
    mat4 world_matrix = c_worldMatrix;
#endif
    vec4 world_position = world_matrix * vec4(in_position, 1.0);

    mat3 rotation = mat3(world_matrix);
    vec3 T = normalize(rotation * in_tangent);
    vec3 N = normalize(rotation * in_normal);
    vec3 B = cross(N, T);

    gl_Position = c_projectionMatrix * (c_viewMatrix * world_position);

    ps_in.position = world_position.xyz;
    ps_in.uv = in_uv;
    ps_in.T = T;
    ps_in.B = B;
    ps_in.N = N;
}
