/// File: gui.ps.glsl
#include "../cbuffer.hlsl.h"
layout(location = 0) in vec4 pass_color;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = pass_color;
}
