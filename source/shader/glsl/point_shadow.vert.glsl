#include "../hlsl/cbuffer.h"

layout(location = 0) in vec3 in_position;
layout(location = 4) in ivec4 in_bone_id;
layout(location = 5) in vec4 in_bone_weight;

out vec3 pass_position;

void main() {
#ifdef HAS_ANIMATION
    mat4 bone_matrix = g_bones[in_bone_id.x] * in_bone_weight.x;
    bone_matrix += g_bones[in_bone_id.y] * in_bone_weight.y;
    bone_matrix += g_bones[in_bone_id.z] * in_bone_weight.z;
    bone_matrix += g_bones[in_bone_id.w] * in_bone_weight.w;
    vec4 position = g_world * bone_matrix * vec4(in_position, 1.0);
#else
    vec4 position = g_world * vec4(in_position, 1.0);
#endif

    pass_position = position.xyz;
    gl_Position = g_projection_view * position;
}
