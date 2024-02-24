#include "cbuffer.glsl.h"

layout(location = 0) in vec3 in_position;

out vec3 pass_position;

void main() {
    mat4 rotate = mat4(mat3(c_view_matrix));  // remove translation from the view matrix
    vec4 position = c_projection_matrix * rotate * vec4(in_position, 1.0);

    pass_position = in_position;
    gl_Position = position.xyww;
}