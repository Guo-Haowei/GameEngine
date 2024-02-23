layout(location = 0) in vec2 pass_uv;
layout(location = 0) out vec4 out_color;

#include "cbuffer.glsl.h"

void main() {
    vec4 color = texture(c_debug_draw_map, pass_uv);

    switch (c_display_channel) {
        case DISPLAY_CHANNEL_RGB:
            out_color = vec4(color.rgb, 1.0);
            break;
        case DISPLAY_CHANNEL_RRR:
            out_color = vec4(color.rrr, 1.0);
            break;
        case DISPLAY_CHANNEL_AAA:
            out_color = vec4(color.aaa, 1.0);
            break;
        default:
            break;
    }
}
