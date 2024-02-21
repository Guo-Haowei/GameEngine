layout(location = 0) out vec4 out_color;

in struct PS_INPUT {
    vec3 position;
    vec2 uv;
    mat3 TBN;
} ps_in;

#include "cbuffer.glsl.h"

void main() {
    vec4 albedo = c_albedo_color;

    if (c_has_albedo_map != 0) {
        albedo = texture(c_albedo_map, ps_in.uv, 0);
    }
    if (albedo.a < 0.001) {
        discard;
    }

    out_color = albedo;
}
