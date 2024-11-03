#include "pbr.glsl"
#include "shadow.glsl"

// @TODO: refactor
vec3 lighting(vec3 N, vec3 L, vec3 V, vec3 radiance, vec3 F0, float roughness, float metallic, vec3 p_base_color) {
    vec3 Lo = vec3(0.0, 0.0, 0.0);
    const vec3 H = normalize(V + L);
    const float NdotL = max(dot(N, L), 0.0);
    const float NdotH = max(dot(N, H), 0.0);
    const float NdotV = max(dot(N, V), 0.0);

    // direct cook-torrance brdf
    const float NDF = distribution_ggx(NdotH, roughness);
    const float G = geometry_smith(NdotV, NdotL, roughness);
    const vec3 F = fresnel_schlick(clamp(dot(H, V), 0.0, 1.0), F0);

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