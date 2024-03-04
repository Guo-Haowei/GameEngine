#include "../cbuffer.h"

layout(location = 0) in vec3 in_position;
layout(location = 2) in vec2 in_uv;

out vec2 pass_uv;

void main() {
    pass_uv = in_uv;

    mat4 view_model = u_view_matrix;
#if 1
    view_model[0][0] = 1.0;
    view_model[0][1] = 0.0;
    view_model[0][2] = 0.0;
    view_model[1][0] = 0.0;
    view_model[1][1] = 1.0;
    view_model[1][2] = 0.0;
    view_model[2][0] = 0.0;
    view_model[2][1] = 0.0;
    view_model[2][2] = 1.0;
#endif

    gl_Position = u_proj_matrix * view_model * vec4(in_position, 1.0);
}
