/// File: debug_draw.vs.glsl
#include "../cbuffer.hlsl.h"
#include "../vsinput.glsl.h"

layout(location = 0) out vec4 pass_color;

void main() {
    gl_Position = c_camProj * (c_camView * vec4(in_position, 1.0));
    pass_color = in_color;
}
