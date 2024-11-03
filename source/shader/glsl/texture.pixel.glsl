#include "../cbuffer.h"

layout(location = 0) in vec2 pass_uv;
layout(location = 0) out vec4 out_color;

void main() {
    vec4 color = texture(c_grassBaseColor, pass_uv);
    if (color.a < 0.01) {
        discard;
    }
    out_color = color;
}