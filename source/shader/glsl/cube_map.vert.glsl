#include "../cbuffer.h"

layout(location = 0) in vec3 in_position;

out vec3 pass_position;

void main() {
    pass_position = in_position;
    vec4 world_position = vec4(in_position, 1.0);
    gl_Position = g_cube_projection_view_matrix * world_position;
}
