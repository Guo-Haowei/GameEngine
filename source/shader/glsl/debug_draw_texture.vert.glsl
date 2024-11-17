/// File: debug_draw_texture.vert.glsl
layout(location = 0) in vec2 in_position;
layout(location = 0) out vec2 pass_uv;

#include "../cbuffer.hlsl.h"

void main() {
    vec2 position = in_position;
    position *= c_debugDrawSize;
    position += c_debugDrawPos;
    gl_Position = vec4(position, -1.0, 1.0);
    pass_uv = 0.5 * (in_position + 1.0);
}
