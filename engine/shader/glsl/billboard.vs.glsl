/// File: billboard.vs.glsl
#include "../cbuffer.hlsl.h"

layout(location = 0) in vec3 in_position;
layout(location = 2) in vec2 in_uv;

out vec2 pass_uv;

void main() {
    pass_uv = in_uv;

    float x_offset = float(gl_InstanceID / 8);
    float z_offset = float(gl_InstanceID % 8);

    vec4 position = vec4(in_position, 1.0);
    position.x += 0.3 * x_offset;
    position.z += 0.3 * z_offset;

    mat4 view_model = c_camView;
#if 0
    view_model[0][0] = 1.0;
    view_model[0][1] = 0.0;
    view_model[0][2] = 0.0;
    view_model[1][0] = 0.0;
    view_model[1][1] = 1.0;
    view_model[1][2] = 0.0;
    view_model[2][0] = 0.0;
    view_model[2][1] = 0.0;
    view_model[2][2] = 1.0;
#endif

    gl_Position = c_camProj * view_model * position;
}
