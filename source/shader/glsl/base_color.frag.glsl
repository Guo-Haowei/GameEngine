layout(location = 0) out vec4 out_color;

in struct PS_INPUT {
    vec3 position;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
} ps_in;

#include "../cbuffer.h"

void main() {
    vec4 albedo = u_albedo_color;

    if (u_has_albedo_map != 0) {
        albedo = texture(u_albedo_map, ps_in.uv, 0);
    }
    if (albedo.a < 0.001) {
        discard;
    }

    out_color = albedo;
}
