#ifndef SHADOW_GLSL_INCLUDED
#define SHADOW_GLSL_INCLUDED

#define NUM_POINT_SHADOW_SAMPLES 20

vec3 POINT_LIGHT_SHADOW_SAMPLE_OFFSET[NUM_POINT_SHADOW_SAMPLES] = vec3[](
    vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1));

float point_shadow_calculation(vec3 p_frag_pos, int p_light_index, vec3 p_eye) {
    vec3 light_position = u_lights[p_light_index].position;
    float light_far = u_lights[p_light_index].max_distance;

    vec3 frag_to_light = p_frag_pos - light_position;
    float current_depth = length(frag_to_light);

    float bias = 0.01;

    float view_distance = length(p_eye - p_frag_pos);
    float disk_radius = (1.0 + (view_distance / light_far)) / 100.0;
    // float disk_radius = (1.0 + (view_distance / light_far)) / 25.0;
    float shadow = 0.0;
    for (int i = 0; i < NUM_POINT_SHADOW_SAMPLES; ++i) {
        float closest_depth = texture(u_lights[p_light_index].shadow_map, frag_to_light + POINT_LIGHT_SHADOW_SAMPLE_OFFSET[i] * disk_radius).r;
        closest_depth *= light_far;
        if (current_depth - bias > closest_depth) {
            shadow += 1.0;
        }
    }
    shadow /= float(NUM_POINT_SHADOW_SAMPLES);

    return shadow;
}

float shadow_test_voxel(sampler2D p_shadow_map,
                        const in vec3 p_pos_world,
                        float p_NdotL) {
    vec4 pos_light = u_main_light_matrices * vec4(p_pos_world, 1.0);
    vec3 coords = pos_light.xyz / pos_light.w;
    coords = 0.5 * coords + 0.5;  // [0, 1]
    float current_depth = coords.z;

    if (current_depth > 1.0) {
        return 0.0;
    }

    float shadow = 0.0;
    vec2 texel_size = 1.0 / vec2(textureSize(p_shadow_map, 0));

    float bias = max(0.005 * (1.0 - p_NdotL), 0.0005);
    float closest_depth = texture(p_shadow_map, coords.xy).r;
    shadow += current_depth - bias > closest_depth ? 1.0 : 0.0;

    return shadow;
}

float shadow_test(sampler2D p_shadow_map,
                  const in vec3 p_pos_world,
                  float p_NdotL) {
    vec4 pos_light = u_main_light_matrices * vec4(p_pos_world, 1.0);
    vec3 coords = pos_light.xyz / pos_light.w;
    coords = 0.5 * coords + 0.5;  // [0, 1]
    float current_depth = coords.z;

    if (current_depth > 1.0) {
        return 0.0;
    }

    float shadow = 0.0;
    vec2 texel_size = 1.0 / vec2(textureSize(p_shadow_map, 0));

    // @TODO: better bias
    float bias = max(0.005 * (1.0 - p_NdotL), 0.0005);

    const int SAMPLE_STEP = 1;
    for (int x = -SAMPLE_STEP; x <= SAMPLE_STEP; ++x) {
        for (int y = -SAMPLE_STEP; y <= SAMPLE_STEP; ++y) {
            vec2 offset = vec2(x, y) * texel_size;
            float closest_depth = texture(p_shadow_map, coords.xy + offset).r;
            shadow += current_depth - bias > closest_depth ? 1.0 : 0.0;
        }
    }

    const float samples = float(2 * SAMPLE_STEP + 1);
    shadow /= samples * samples;
    return shadow;
}

#endif