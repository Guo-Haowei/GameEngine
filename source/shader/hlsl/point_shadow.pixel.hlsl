#include "cbuffer.h"
#include "hlsl/input_output.hlsl"

float main(float4 position
           : SV_POSITION) : SV_DEPTH {
    float light_distance = length(position - g_point_light_position);
    light_distance = light_distance / g_point_light_far;
    return light_distance;
}