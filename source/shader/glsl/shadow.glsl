#ifndef SHADOW_GLSL_INCLUDED
#define SHADOW_GLSL_INCLUDED
#include "../shadow.hlsl"

#define NUM_POINT_SHADOW_SAMPLES 20

vec3 POINT_LIGHT_SHADOW_SAMPLE_OFFSET[NUM_POINT_SHADOW_SAMPLES] = vec3[](
    vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1));

float point_shadow_calculation(Light p_light, vec3 p_frag_pos, vec3 p_eye) {
    vec3 light_position = p_light.position;
    float light_far = p_light.max_distance;

    samplerCube point_shadow_map;
    switch (p_light.shadow_map_index) {
        case 0:
            point_shadow_map = t_point_shadow_0;
            break;
        case 1:
            point_shadow_map = t_point_shadow_1;
            break;
        case 2:
            point_shadow_map = t_point_shadow_2;
            break;
        case 3:
            point_shadow_map = t_point_shadow_3;
            break;
        default:
            break;
    }

    vec3 frag_to_light = p_frag_pos - light_position;
    float current_depth = length(frag_to_light);

    float bias = 0.01;

    float view_distance = length(p_eye - p_frag_pos);
    float disk_radius = (1.0 + (view_distance / light_far)) / 100.0;
    float shadow = 0.0;
    for (int i = 0; i < NUM_POINT_SHADOW_SAMPLES; ++i) {
        // @HACK
        float closest_depth = texture(point_shadow_map, frag_to_light + POINT_LIGHT_SHADOW_SAMPLE_OFFSET[i] * disk_radius).r;
        closest_depth *= light_far;
        if (current_depth - bias > closest_depth) {
            shadow += 1.0;
        }
    }
    shadow /= float(NUM_POINT_SHADOW_SAMPLES);

    return shadow;
}

#endif