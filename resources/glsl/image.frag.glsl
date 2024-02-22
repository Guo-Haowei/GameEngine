layout(location = 0) in vec2 pass_uv;
layout(location = 0) out vec4 out_color;

#include "cbuffer.glsl.h"

void main() {
    vec3 color = vec3(0.0);
    switch (c_display_channel) {
        case DISPLAY_CHANNEL_RGB:
            color = texture(c_albedo_map, pass_uv).rgb;
            break;
        case DISPLAY_CHANNEL_RRR:
            color = texture(c_albedo_map, pass_uv).rrr;
            break;
        case DISPLAY_CHANNEL_AAA:
            color = texture(c_albedo_map, pass_uv).aaa;
            break;
        default:
            break;
    }
    out_color = vec4(color, 1.0);
}
