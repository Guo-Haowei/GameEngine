#include "cbuffer.glsl.h"

in vec4 pass_position;

void main() {
    float light_distance = length(pass_position.xyz - c_point_light_position);

    light_distance = light_distance / c_point_light_far;

    // write this as modified depth
    gl_FragDepth = light_distance;
}