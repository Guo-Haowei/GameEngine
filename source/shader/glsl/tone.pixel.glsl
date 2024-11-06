/// File: tone.pixel.glsl
#include "../cbuffer.h"
#include "../texture_binding.h"

layout(location = 0) in vec2 pass_uv;
layout(location = 0) out vec3 out_color;

const mat3 sx = mat3(
    1.0, 2.0, 1.0,
    0.0, 0.0, 0.0,
    -1.0, -2.0, -1.0);
const mat3 sy = mat3(
    1.0, 0.0, -1.0,
    2.0, 0.0, -2.0,
    1.0, 0.0, -1.0);

void main() {
    const vec2 uv = pass_uv;
    const float gamma = 1.0 / 2.2;

    // edge detection
    vec2 texel_size = textureSize(t_selectionHighlight, 0);
    texel_size = 1.0 / texel_size;

    mat3 I;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            vec2 offset = uv + texel_size * vec2(i - 1, j - 1);
            I[i][j] = texture(t_selectionHighlight, offset).r;
        }
    }

    float gx = dot(sx[0], I[0]) + dot(sx[1], I[1]) + dot(sx[2], I[2]);
    float gy = dot(sy[0], I[0]) + dot(sy[1], I[1]) + dot(sy[2], I[2]);

    float g = sqrt(pow(gx, 2.0) + pow(gy, 2.0));
    if (g > 0.0) {
        out_color = vec3(0.98, 0.64, 0);
        return;
    }

    // @TODO: add bloom
    vec3 color = texture(t_textureLighting, uv).rgb;

    if (c_enableBloom == 1) {
        vec3 bloom = texture(c_finalBloom, uv).rgb;
        color += bloom;
    }

    color = color / (color + 1.0);
    color = pow(color, vec3(gamma));

    out_color = color;
}
