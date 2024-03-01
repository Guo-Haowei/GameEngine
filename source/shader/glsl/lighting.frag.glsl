#include "../hlsl/cbuffer.h"

#define ENABLE_VXGI 1

layout(location = 0) out vec3 out_color;
layout(location = 0) in vec2 pass_uv;

#include "lighting.glsl"

#if ENABLE_VXGI
#include "vxgi.glsl"
#endif

vec3 FresnelSchlickRoughness(float cosTheta, in vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness) - F0, vec3(0.0))) * pow(1.0 - cosTheta, 5.0);
}

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

vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec4 points[4], bool twoSided) {
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
    float scale = texture(u_ltc_2, texcoord).w;

    float sum = len * scale;
    if (!behind && !twoSided)
        sum = 0.0;

    // Outgoing radiance (solid angle) for the entire polygon
    vec3 Lo_i = vec3(sum, sum, sum);
    return Lo_i;
}

void main() {
    const vec2 texcoord = pass_uv;
    float depth = texture(u_gbuffer_depth_map, texcoord).r;

    if (depth > 0.999) discard;

    gl_FragDepth = depth;

    vec3 N = texture(u_gbuffer_normal_map, texcoord).rgb;
    // N = (2.0 * N) - vec3(1.0);

    const vec3 world_position = texture(u_gbuffer_position_map, texcoord).rgb;
    const vec3 emissive_roughness_metallic = texture(u_gbuffer_material_map, texcoord).rgb;
    float emissive = emissive_roughness_metallic.r;
    float roughness = emissive_roughness_metallic.g;
    float metallic = emissive_roughness_metallic.b;

    vec3 base_color = texture(u_gbuffer_base_color_map, texcoord).rgb;
    if (c_no_texture != 0) {
        base_color = vec3(0.6);
    }

    // @TODO: move this before those heavy calculation
    if (emissive > 0.0) {
        out_color = emissive * base_color;
        return;
    }

    const int cascade_level = find_cascade(world_position);

    const vec3 V = normalize(c_camera_position - world_position);
    const float NdotV = clamp(dot(N, V), 0.0, 1.0);
    vec3 R = reflect(-V, N);

    vec3 Lo = vec3(0.0);
    vec3 F0 = mix(vec3(0.04), base_color, metallic);

    // ------------------- for area light
    // use roughness and sqrt(1-cos_theta) to sample M_texture
    vec2 uv = vec2(roughness, sqrt(1.0f - NdotV));
    uv = uv * LUT_SCALE + LUT_BIAS;

    // get 4 parameters for inverse_M
    vec4 t1 = texture(u_ltc_1, uv);

    // Get 2 parameters for Fresnel calculation
    vec4 t2 = texture(u_ltc_2, uv);

    mat3 Minv = mat3(
        vec3(t1.x, 0, t1.y),
        vec3(0, 1, 0),
        vec3(t1.z, 0, t1.w));
    // ------------------- for area light

    for (int light_idx = 0; light_idx < c_light_count; ++light_idx) {
        Light light = c_lights[light_idx];
        int light_type = c_lights[light_idx].type;
        vec3 direct_lighting = vec3(0.0);
        float shadow = 0.0;
        switch (light.type) {
            case LIGHT_TYPE_OMNI: {
                vec3 L = light.position;
                float atten = 1.0;

                const vec3 H = normalize(V + L);
                const vec3 radiance = light.color;
                direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
                if (light.cast_shadow == 1) {
                    const float NdotL = max(dot(N, L), 0.0);
                    shadow = cascade_shadow(c_shadow_map, world_position, NdotL, cascade_level);
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
                    const vec3 radiance = c_lights[light_idx].color;
                    direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
                    if (light.cast_shadow == 1) {
                        shadow = point_shadow_calculation(world_position, light_idx, c_camera_position);
                    }
                }
            } break;
            case LIGHT_TYPE_AREA: {
                // Evaluate LTC shading
                vec3 diffuse = LTC_Evaluate(N, V, world_position, mat3(1), c_lights[light_idx].points, false);
                vec3 specular = LTC_Evaluate(N, V, world_position, Minv, c_lights[light_idx].points, false);

                const vec3 kS = F0;
                const vec3 kD = base_color;

                // GGX BRDF shadowing and Fresnel
                // t2.x: shadowedF90 (F90 normally it should be 1.0)
                // t2.y: Smith function for Geometric Attenuation Term, it is dot(V or L, H).
                specular *= kS * t2.x + (1.0f - kS) * t2.y;

                // assume no shadow
                // Add contribution
                direct_lighting = c_lights[light_idx].color * (specular + kD * diffuse);
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
    vec3 irradiance = texture(c_diffuse_irradiance_map, N).rgb;
    vec3 diffuse = irradiance * base_color.rgb;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(c_prefiltered_map, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(c_brdf_map, vec2(NdotV, roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    const float ao = c_enable_ssao == 0 ? 1.0 : texture(c_ssao_map, texcoord).r;
    vec3 ambient = (kD * diffuse + specular) * ao;

#if ENABLE_VXGI
    if (c_enable_vxgi == 1) {
        const vec3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
        const vec3 kS = F;
        const vec3 kD = (1.0 - kS) * (1.0 - metallic);

        // indirect diffuse
        vec3 diffuse = base_color.rgb * cone_diffuse(world_position, N);

        // specular cone
        vec3 coneDirection = reflect(-V, N);
        vec3 specular = vec3(0);
        specular = metallic * cone_specular(world_position, coneDirection, roughness);
        Lo += (kD * diffuse + specular) * ao;
    }
#endif

    vec3 color = Lo + ambient;

    // debug CSM
    if (c_debug_csm == 1) {

        float alpha = 0.2;
        switch (cascade_level) {
            case 0:
                color = mix(color, vec3(1, 0, 0), alpha);
                break;
            case 1:
                color = mix(color, vec3(0, 1, 0), alpha);
                break;
            case 2:
                color = mix(color, vec3(0, 0, 1), alpha);
                break;
            default:
                color = mix(color, vec3(1, 1, 0), alpha);
                break;
        }
    }

#if ENABLE_CSM
    if (c_debug_csm != 0) {
        vec3 mask = vec3(0.1);
        for (int light_idx = 0; light_idx < MAX_CASCADE_COUNT; ++light_idx) {
            if (clipSpaceZ <= c_cascade_clip_z[light_idx + 1]) {
                mask[light_idx] = 0.7;
                break;
            }
        }
        color = mix(color.rgb, mask, 0.1);
    }
#endif

    out_color = color;
}
