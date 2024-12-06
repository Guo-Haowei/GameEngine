/// File: skybox.ps.glsl
#include "../cbuffer.hlsl.h"

in vec3 pass_position;
out vec4 out_color;

#if 0
void main() {
    vec3 color = texture(c_envMap, pass_position).rgb;
    // vec3 color = texture(c_diffuseIrradianceMap, pass_position).rgb;
    // vec3 color = textureLod(c_prefilteredMap, pass_position, 1.0).rgb;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    out_color = vec4(color, 1.0);
    // out_color = vec4(1.0);
}
#endif

vec2 sample_spherical_map(in vec3 v) {
    const vec2 inv_atan = vec2(0.1591, 0.3183);
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= inv_atan;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

void main() {
    vec2 uv = sample_spherical_map(normalize(pass_position));
    vec3 color = texture(c_hdrEnvMap, uv).rgb;
    out_color = vec4(color, 1.0);
}
