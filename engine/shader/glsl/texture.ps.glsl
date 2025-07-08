/// File: texture.ps.glsl
#include "../cbuffer.hlsl.h"

layout(location = 0) in vec2 pass_uv;
layout(location = 0) out vec4 out_color;

void main() {
    vec4 color = texture(xxx, pass_uv);
    if (color.a < 0.01) {
        discard;
    }
    out_color = color;
}