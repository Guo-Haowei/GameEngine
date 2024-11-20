/// File: shadowmap_point.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

float main(vsoutput_position input) : SV_DEPTH {
    float light_distance = length(input.world_position - c_pointLightPosition);
    light_distance = light_distance / c_pointLightFar;
    return light_distance;
}