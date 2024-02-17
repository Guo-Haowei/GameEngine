float distributionGGX(float NdotH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH2 = NdotH * NdotH;

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}

float geometrySmith(float NdotV, float NdotL, float roughness) {
    return geometrySchlickGGX(NdotV, roughness) * geometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(float cosTheta, const in vec3 F0) { return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); }

// no filter
float Shadow(sampler2D shadowMap, const in vec4 position_light, float NdotL
             //, int level
) {
    vec3 coords = position_light.xyz / position_light.w;
    coords = 0.5 * coords + 0.5;

    // coords.x /= float( SC_NUM_CASCADES );
    // coords.x += float( level ) / float( SC_NUM_CASCADES );

    float current_depth = coords.z;

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    float bias = max(0.005 * (1.0 - NdotL), 0.0005);

    const int SAMPLE_STEP = 1;
    for (int x = -SAMPLE_STEP; x <= SAMPLE_STEP; ++x) {
        for (int y = -SAMPLE_STEP; y <= SAMPLE_STEP; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float closest_depth = texture(shadowMap, coords.xy + offset).r;

            shadow += coords.z - bias > closest_depth ? 1.0 : 0.0;
        }
    }

    const float samples = float(2 * SAMPLE_STEP + 1);
    shadow /= samples * samples;
    return shadow;
}

// @TODO: refactor
vec3 lighting(vec3 N, vec3 L, vec3 V, vec3 radiance, vec3 F0, float roughness, float metallic, vec4 albedo) {
    vec3 Lo = vec3(0.0, 0.0, 0.0);
    const vec3 H = normalize(V + L);
    const float NdotL = max(dot(N, L), 0.0);
    const float NdotH = max(dot(N, H), 0.0);
    const float NdotV = max(dot(N, V), 0.0);

    // direct cook-torrance brdf
    const float NDF = distributionGGX(NdotH, roughness);
    const float G = geometrySmith(NdotV, NdotL, roughness);
    const vec3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    const vec3 nom = NDF * G * F;
    const float denom = 4 * NdotV * NdotL;

    vec3 specular = nom / max(denom, 0.001);

    const vec3 kS = F;
    const vec3 kD = (1.0 - metallic) * (vec3(1.0) - kS);

    vec3 direct_lighting = (kD * albedo.rgb / PI + specular) * radiance * NdotL;

    return direct_lighting;
    //     //
    //     float shadow = 0.0;
    // #if ENABLE_CSM
    //     // float clipSpaceZ = ( c_projection_view_matrix * worldPos ).z;
    //     // for ( int idx = 0; idx < SC_NUM_CASCADES; ++idx )
    //     // {
    //     //     if ( clipSpaceZ <= c_cascade_clip_z[idx + 1] )
    //     //     {
    //     //         vec4 lightSpacePos = c_light_matricies[idx] * worldPos;
    //     //         shadow             = Shadow( c_shadow_map, lightSpacePos, NdotL, idx );
    //     //         break;
    //     //     }
    //     // }
    // #else
    //     vec4 lightSpacePos = c_sun_light_matricies[0] * vec4(world_position, 1.0);
    //     shadow = Shadow(c_shadow_map, lightSpacePos, NdotL);
    // #endif
    //     Lo += (1.0 - shadow) * directLight;

    //     // @TODO: HACK
    //     Lo += 0.2 * albedo.rgb;  // ambient
    //     return Lo;
}
