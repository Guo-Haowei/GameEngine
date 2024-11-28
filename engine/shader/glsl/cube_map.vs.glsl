/// File: cube_map.vs.glsl
#include "../cbuffer.hlsl.h"

layout(location = 0) in vec3 in_position;

out vec3 pass_position;

void main() {
    pass_position = in_position;
    vec4 world_position = vec4(in_position, 1.0);
    gl_Position = c_cubeProjectionViewMatrix * world_position;
}
