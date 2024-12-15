/// File: gui.vs.glsl
#include "../cbuffer.hlsl.h"

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec4 in_color;

layout(location = 0) out vec4 pass_color;

void main() {
    gl_Position = c_projectionMatrix * (c_viewMatrix * vec4(in_position, 1.0));
    pass_color = in_color;
}
