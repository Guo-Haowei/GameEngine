#include "../cbuffer.hlsl.h"

layout(location = 0) in vec3 in_position;

out vec3 pass_pos;
out vec3 pass_fsun;

uniform float time = 10.0;

void main() {
    gl_Position = vec4(in_position.xy, 0.0, 1.0);
    pass_pos = transpose(mat3(c_viewMatrix)) * (c_invProjection * gl_Position).xyz;
    // @TODO: actual sun direction
    pass_fsun = vec3(0.0, sin(time * 0.01), cos(time * 0.01));
}