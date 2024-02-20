#include "cbuffer.glsl.h"

in vec4 pass_position;

void main() {
    float light_distance = length(pass_position.xyz - c_lights[c_light_index].position);

    light_distance = light_distance / c_lights[c_light_index].max_distance;

    // write this as modified depth
    gl_FragDepth = light_distance;
}