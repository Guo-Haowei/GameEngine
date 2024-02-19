layout(location = 0) in vec3 in_position;

out vec3 pass_position;

#include "cbuffer.glsl.h"

void main() {
    // pass_position = in_position;
    // vec4 world_position = vec4(in_position, 1.0);
    // gl_Position = u_per_frame.projection * u_per_frame.view * world_position;
}
