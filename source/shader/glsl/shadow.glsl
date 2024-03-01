#ifndef SHADOW_GLSL_INCLUDED
#define SHADOW_GLSL_INCLUDED

int find_cascade(const in vec3 p_pos_world) {
    if (c_enable_csm == 0) {
        return 0;
    }

    vec4 pos_view = u_view_matrix * vec4(p_pos_world, 1.0);
    float depth = abs(pos_view.z);

    for (int i = 0; i < MAX_CASCADE_COUNT; ++i) {
        if (depth < c_cascade_plane_distances[i]) {
            return i;
        }
    }

    return MAX_CASCADE_COUNT - 1;
}

#define NUM_POINT_SHADOW_SAMPLES 20

vec3 g_point_light_shadow_sample_offset[NUM_POINT_SHADOW_SAMPLES] = vec3[](
    vec3(1, 1, 1), vec3(1, -1, 1), vec3(-1, -1, 1), vec3(-1, 1, 1),
    vec3(1, 1, -1), vec3(1, -1, -1), vec3(-1, -1, -1), vec3(-1, 1, -1),
    vec3(1, 1, 0), vec3(1, -1, 0), vec3(-1, -1, 0), vec3(-1, 1, 0),
    vec3(1, 0, 1), vec3(-1, 0, 1), vec3(1, 0, -1), vec3(-1, 0, -1),
    vec3(0, 1, 1), vec3(0, -1, 1), vec3(0, -1, -1), vec3(0, 1, -1));

float point_shadow_calculation(vec3 p_frag_pos, int p_light_index, vec3 p_eye) {
    vec3 light_position = c_lights[p_light_index].position;
    float light_far = c_lights[p_light_index].max_distance;

    vec3 frag_to_light = p_frag_pos - light_position;
    float current_depth = length(frag_to_light);

    float bias = 0.01;

    float view_distance = length(p_eye - p_frag_pos);
    float disk_radius = (1.0 + (view_distance / light_far)) / 100.0;
    // float disk_radius = (1.0 + (view_distance / light_far)) / 25.0;
    float shadow = 0.0;
    for (int i = 0; i < NUM_POINT_SHADOW_SAMPLES; ++i) {
        float closest_depth = texture(c_lights[p_light_index].shadow_map, frag_to_light + g_point_light_shadow_sample_offset[i] * disk_radius).r;
        closest_depth *= light_far;
        if (current_depth - bias > closest_depth) {
            shadow += 1.0;
        }
    }
    shadow /= float(NUM_POINT_SHADOW_SAMPLES);

    return shadow;
}

float cascade_shadow_voxel(sampler2D p_shadow_map,
                           const in vec3 p_pos_world,
                           float p_NdotL,
                           int p_level) {
    vec4 pos_light = c_main_light_matrices[p_level] * vec4(p_pos_world, 1.0);
    vec3 coords = pos_light.xyz / pos_light.w;
    coords = 0.5 * coords + 0.5;  // [0, 1]
    float current_depth = coords.z;
    coords.x += p_level;
    coords.x /= MAX_CASCADE_COUNT;

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

float cascade_shadow(sampler2D p_shadow_map,
                     const in vec3 p_pos_world,
                     float p_NdotL,
                     int p_level) {
    vec4 pos_light = c_main_light_matrices[p_level] * vec4(p_pos_world, 1.0);
    vec3 coords = pos_light.xyz / pos_light.w;
    coords = 0.5 * coords + 0.5;  // [0, 1]
    float current_depth = coords.z;
    coords.x += p_level;
    coords.x /= MAX_CASCADE_COUNT;

#if 0
    float lower = 0.001;
    float upper = 0.999;
    if (coords.x < lower || coords.x > upper) {
        return 0.0;
    }
    if (coords.y < lower || coords.y > upper) {
        return 0.0;
    }
#endif

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