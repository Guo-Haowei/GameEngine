layout(location = 0) in vec2 in_position;
layout(location = 0) out vec2 pass_uv;

#include "cbuffer.glsl.h"

void main() {
    vec2 position = in_position;
    position *= c_debug_draw_size;
    position += c_debug_draw_pos;
    gl_Position = vec4(position, -1.0, 1.0);
    pass_uv = 0.5 * (in_position + 1.0);
}
