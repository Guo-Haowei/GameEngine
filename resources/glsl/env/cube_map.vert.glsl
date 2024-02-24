#include "cbuffer.glsl.h"

layout(location = 0) in vec3 in_position;

out vec3 pass_position;

void main() {
    pass_position = in_position;
    vec4 world_position = vec4(in_position, 1.0);
    // @TODO: fix this
    gl_Position = c_projection_view_model_matrix * world_position;
}
