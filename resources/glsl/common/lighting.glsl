#include "common/pbr.glsl"
#include "common/shadow.glsl"

// @TODO: refactor
vec3 lighting(vec3 N, vec3 L, vec3 V, vec3 radiance, vec3 F0, float roughness, float metallic, vec4 albedo) {
    vec3 Lo = vec3(0.0, 0.0, 0.0);
    const vec3 H = normalize(V + L);
    const float NdotL = max(dot(N, L), 0.0);
    const float NdotH = max(dot(N, H), 0.0);
    const float NdotV = max(dot(N, V), 0.0);

    // direct cook-torrance brdf
    const float NDF = distributionGGX(NdotH, roughness);
    const float G = GeometrySmith(NdotV, NdotL, roughness);
    const vec3 F = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    const vec3 nom = NDF * G * F;
    const float denom = 4 * NdotV * NdotL;

    vec3 specular = nom / max(denom, 0.001);

    const vec3 kS = F;
    const vec3 kD = (1.0 - metallic) * (vec3(1.0) - kS);

    vec3 direct_lighting = (kD * albedo.rgb / MY_PI + specular) * radiance * NdotL;

    return direct_lighting;
}
