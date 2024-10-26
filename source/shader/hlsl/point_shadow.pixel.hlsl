#include "cbuffer.h"
#include "hlsl/input_output.hlsl"

float main(vsoutput_position input) : SV_DEPTH {
    float light_distance = length(input.world_position - g_point_light_position);
    light_distance = light_distance / g_point_light_far;
    return light_distance;
}