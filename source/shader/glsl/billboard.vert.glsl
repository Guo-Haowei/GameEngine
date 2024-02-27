#include "../cbuffer.h"

layout(location = 0) in vec3 in_position;
layout(location = 2) in vec2 in_texcoord_0;

out vec2 pass_texcoord_0;

void main() {
    pass_texcoord_0 = in_texcoord_0;
    mat4 view_model = c_view_matrix * c_model_matrix;
    view_model[0][0] = 1.0;
    view_model[0][1] = 0.0;
    view_model[0][2] = 0.0;
    view_model[1][0] = 0.0;
    view_model[1][1] = 1.0;
    view_model[1][2] = 0.0;
    view_model[2][0] = 0.0;
    view_model[2][1] = 0.0;
    view_model[2][2] = 1.0;

    gl_Position = c_projection_matrix * view_model * vec4(in_position, 1.0);
}
