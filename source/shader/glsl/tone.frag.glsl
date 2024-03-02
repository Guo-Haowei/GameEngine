#include "../hlsl/cbuffer.h"

layout(location = 0) in vec2 pass_uv;
layout(location = 0) out vec3 out_color;

// @TODO: move to genera place
float rgb_to_luma(vec3 rgb) {
    return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}

void main() {
    const vec2 uv = pass_uv;
    const float gamma = 1.0 / 2.2;

    // @TODO: add bloom
    vec3 color = texture(c_tone_input_image, uv).rgb;

    if (c_enable_bloom == 1) {
        vec3 bloom = texture(u_final_bloom, uv).rgb;
        color += bloom;
    }

    color = color / (color + 1.0);
    color = pow(color, vec3(gamma));

    // float luma = rgb_to_luma(pixel);
    out_color = color;
}
