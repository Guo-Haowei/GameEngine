#include "../cbuffer.h"

layout(location = 0) in vec2 pass_texcoord_0;
layout(location = 0) out vec4 out_color;

void main() {
    vec4 color = texture(c_albedo_map, pass_texcoord_0);
    if (color.a < 0.01) {
        discard;
    }
    out_color = color;
}