#include "../cbuffer.h"

layout(location = 0) out vec4 out_color;

in vec3 pass_position;

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
