#include "../cbuffer.h"

in vec3 pass_position;

void main() {
    // @TODO: shit
    float light_distance = length(pass_position - g_point_light_position);

    light_distance = light_distance / g_point_light_far;

    gl_FragDepth = light_distance;
}