/// File: lighting.glsl
#include "../pbr.hlsl.h"
#include "shadow.glsl"

#define ENABLE_VXGI 1
#if ENABLE_VXGI
#include "vxgi.glsl"
#endif

#ifdef DISABLE_IBL
#define ENABLE_IBL 0
#else
#define ENABLE_IBL 1
#endif

// @TODO: refactor
vec3 lighting(vec3 N, vec3 L, vec3 V, vec3 radiance, vec3 F0, float roughness, float metallic, vec3 p_base_color) {
    vec3 Lo = vec3(0.0, 0.0, 0.0);
    const vec3 H = normalize(V + L);
    const float NdotL = max(dot(N, L), 0.0);
    const float NdotH = max(dot(N, H), 0.0);
    const float NdotV = max(dot(N, V), 0.0);

    // direct cook-torrance brdf
    const float NDF = DistributionGGX(NdotH, roughness);
    const float G = GeometrySmith(NdotV, NdotL, roughness);
    const vec3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    const vec3 nom = NDF * G * F;
    float denom = 4 * NdotV * NdotL;

    vec3 specular = nom / max(denom, 0.001);

    const vec3 kS = F;
    const vec3 kD = (1.0 - metallic) * (vec3(1.0) - kS);

    vec3 direct_lighting = (kD * p_base_color / MY_PI + specular) * radiance * NdotL;

    return direct_lighting;
}

// rectangle light
vec3 IntegrateEdgeVec(vec3 v1, vec3 v2) {
    // Using built-in acos() function will result flaws
    // Using fitting result for calculating acos()
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
    float b = 3.4175940 + (4.1616724 + y) * y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5 * inversesqrt(max(1.0 - x * x, 1e-7)) - v;

    return cross(v1, v2) * theta_sintheta;
}

vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec4 points[4]) {
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = Minv * transpose(mat3(T1, T2, N));

    // polygon (allocate 4 vertices for clipping)
    vec3 L[4];

    // transform polygon from LTC back to origin Do (cosine weighted)
    vec3 point0 = points[0].xyz;
    vec3 point1 = points[1].xyz;
    vec3 point2 = points[2].xyz;
    vec3 point3 = points[3].xyz;

    L[0] = Minv * (point0 - P);
    L[1] = Minv * (point1 - P);
    L[2] = Minv * (point2 - P);
    L[3] = Minv * (point3 - P);

    // use tabulated horizon-clipped sphere
    // check if the shading point is behind the light
    vec3 dir = point0 - P;  // LTC space
    vec3 lightNormal = cross(point1 - point0, point3 - point0);
    bool behind = (dot(dir, lightNormal) < 0.0);

    // cos weighted space
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);

    // integrate
    vec3 vsum = vec3(0.0);
    vsum += IntegrateEdgeVec(L[0], L[1]);
    vsum += IntegrateEdgeVec(L[1], L[2]);
    vsum += IntegrateEdgeVec(L[2], L[3]);
    vsum += IntegrateEdgeVec(L[3], L[0]);

    // form factor of the polygon in direction vsum
    float len = length(vsum);

    float z = vsum.z / len;
    if (behind)
        z = -z;

    vec2 texcoord = vec2(z * 0.5f + 0.5f, len);  // range [0, 1]
    texcoord = texcoord * LUT_SCALE + LUT_BIAS;

    // Fetch the form factor for horizon clipping
    float scale = texture(c_ltc2, texcoord).w;

    float sum = len * scale;
    if (!behind)
        sum = 0.0;

    // Outgoing radiance (solid angle) for the entire polygon
    vec3 Lo_i = vec3(sum, sum, sum);
    return Lo_i;
}

vec3 area_light(mat3 Minv, vec3 N, vec3 V, vec3 world_position, vec4 p_t2, vec3 p_specular, vec3 p_diffuse, vec4 p_points[4]) {
    vec3 diffuse = LTC_Evaluate(N, V, world_position, mat3(1), p_points);
    vec3 specular = LTC_Evaluate(N, V, world_position, Minv, p_points);

    const vec3 kS = p_specular;
    const vec3 kD = p_diffuse;

    specular *= kS * p_t2.x + (1.0 - kS) * p_t2.y;
    return (specular + kD * diffuse);
}

vec3 compute_lighting(sampler2D shadow_map,
                      sampler3D voxel,
                      vec3 base_color,
                      vec3 world_position,
                      vec3 N,
                      float metallic,
                      float roughness,
                      float emissive) {
    if (emissive > 0.0) {
        return vec3(emissive * base_color);
    }

    const vec3 V = normalize(c_cameraPosition - world_position);
    const float NdotV = max(dot(N, V), 0.0);
    vec3 R = reflect(-V, N);

    vec3 Lo = vec3(0.0);
    vec3 F0 = mix(vec3(0.04), base_color, metallic);

    // ------------------- for area light
    // use roughness and sqrt(1-cos_theta) to sample M_texture
    vec2 uv = vec2(roughness, sqrt(1.0f - NdotV));
    uv = uv * LUT_SCALE + LUT_BIAS;

    // get 4 parameters for inverse_M
    vec4 t1 = texture(c_ltc1, uv);

    // Get 2 parameters for Fresnel calculation
    vec4 t2 = texture(c_ltc2, uv);

    mat3 Minv = mat3(
        vec3(t1.x, 0, t1.y),
        vec3(0, 1, 0),
        vec3(t1.z, 0, t1.w));
    // ------------------- for area light

    for (int light_idx = 0; light_idx < c_lightCount; ++light_idx) {
        Light light = c_lights[light_idx];
        int light_type = c_lights[light_idx].type;
        vec3 direct_lighting = vec3(0.0);
        float shadow = 0.0;
        const vec3 radiance = light.color;
        switch (light.type) {
            case LIGHT_TYPE_INFINITE: {
                vec3 L = light.position;
                float atten = 1.0;
                const vec3 H = normalize(V + L);
                direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
                if (light.cast_shadow == 1) {
                    const float NdotL = max(dot(N, L), 0.0);
                    shadow = shadowTest(shadow_map, light, world_position, NdotL);
                    direct_lighting *= (1.0 - shadow);
                }
            } break;
            case LIGHT_TYPE_POINT: {
                vec3 delta = -world_position + light.position;
                float dist = length(delta);
                float atten = (light.atten_constant + light.atten_linear * dist +
                               light.atten_quadratic * (dist * dist));
                atten = 1.0 / atten;
                if (atten > 0.01) {
                    vec3 L = normalize(delta);
                    const vec3 H = normalize(V + L);
                    direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
                    if (light.cast_shadow == 1) {
                        shadow = point_shadow_calculation(light, world_position, c_cameraPosition);
                    }
                }
            } break;
            case LIGHT_TYPE_AREA: {
                direct_lighting = radiance * area_light(Minv, N, V, world_position, t2, F0, base_color, light.points);
            } break;
            default:
                break;
        }
        Lo += (1.0 - shadow) * direct_lighting;
    }

    // ambient

    vec3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
#if ENABLE_IBL == 1
    vec3 irradiance = texture(t_DiffuseIrradiance, N).rgb;
    vec3 diffuse = irradiance * base_color.rgb;
#else
    vec3 irradiance = vec3(0);
    vec3 diffuse = vec3(0);
#endif

#if ENABLE_IBL == 1
    vec3 prefilteredColor = textureLod(t_Prefiltered, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 brdf_uv = vec2(NdotV, 1.0 - roughness);
    vec2 brdf = texture(t_BrdfLut, brdf_uv).rg;
    vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);
#else
    vec3 specular = vec3(0);
#endif

    const float ao = 1.0;
    vec3 ambient = (kD * diffuse + specular) * ao;

#if ENABLE_VXGI
    if (c_enableVxgi == 1) {
        const vec3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
        const vec3 kS = F;
        const vec3 kD = (1.0 - kS) * (1.0 - metallic);

        // indirect diffuse
        vec3 diffuse = base_color.rgb * cone_diffuse(voxel, world_position, N);

        // specular cone
        vec3 coneDirection = reflect(-V, N);
        vec3 specular = vec3(0);
        specular = metallic * cone_specular(voxel, world_position, coneDirection, roughness);
        Lo += (kD * diffuse + specular) * ao;
    }
#endif

    vec3 final_color = Lo + ambient;
    return final_color;
}
