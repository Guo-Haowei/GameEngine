int find_cascade(const in vec3 p_pos_world) {
    vec4 pos_view = c_view_matrix * vec4(p_pos_world, 1.0);
    float depth = abs(pos_view.z);

    for (int i = 0; i < SC_NUM_CASCADES; ++i) {
        if (depth < c_cascade_plane_distances[i]) {
            return i;
        }
    }

    return SC_NUM_CASCADES - 1;
}

float cascade_shadow(sampler2D p_shadow_maps,
                     const in vec3 p_pos_world,
                     float p_NdotL,
                     int p_level) {
    vec4 pos_light = c_main_light_matrices[p_level] * vec4(p_pos_world, 1.0);
    vec3 coords = pos_light.xyz / pos_light.w;
    coords = 0.5 * coords + 0.5;  // [0, 1]
    float current_depth = coords.z;
    if (current_depth > 1.0) {
        return 0.0;
    }

    float closest_depth = texture(p_shadow_maps, coords.xy).r;
    return current_depth > closest_depth ? 1.0 : 0.0;

#if 0
    float shadow = 0.0;
    ivec2 i_texel_size = textureSize(p_shadow_maps, 0);
    vec2 texel_size = 1.0 / vec2(i_texel_size.x, i_texel_size.y);

    float bias = max(0.005 * (1.0 - p_NdotL), 0.0005);

    const int SAMPLE_STEP = 1;
    for (int x = -SAMPLE_STEP; x <= SAMPLE_STEP; ++x) {
        for (int y = -SAMPLE_STEP; y <= SAMPLE_STEP; ++y) {
            vec2 offset = vec2(x, y) * texel_size;
            float closest_depth = texture(p_shadow_maps, coords.xy + offset).r;
            shadow += current_depth - bias > closest_depth ? 1.0 : 0.0;
        }
    }

    const float samples = float(2 * SAMPLE_STEP + 1);
    shadow /= samples * samples;
    return shadow;
#endif
}
