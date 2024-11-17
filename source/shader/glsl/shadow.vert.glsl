/// File: shadow.vert.glsl
#include "../cbuffer.hlsl.h"

layout(location = 0) in vec3 in_position;
layout(location = 4) in ivec4 in_bone_id;
layout(location = 5) in vec4 in_bone_weight;

void main() {
#ifdef HAS_ANIMATION
    mat4 bone_matrix = c_bones[in_bone_id.x] * in_bone_weight.x;
    bone_matrix += c_bones[in_bone_id.y] * in_bone_weight.y;
    bone_matrix += c_bones[in_bone_id.z] * in_bone_weight.z;
    bone_matrix += c_bones[in_bone_id.w] * in_bone_weight.w;
    mat4 world_matrix = c_worldMatrix * bone_matrix;
#else
    mat4 world_matrix = c_worldMatrix;
#endif

    gl_Position = c_projectionMatrix * c_viewMatrix * world_matrix * vec4(in_position, 1.0);
}
