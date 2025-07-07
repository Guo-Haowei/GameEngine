/// File: visualization.ps.glsl
layout(location = 0) in vec4 pass_color;
layout(location = 0) out vec4 out_color;

#include "../cbuffer.hlsl.h"

void main() {
    if (c_debugVoxelId == 1)  // abledo
    {
        out_color.rgb = 0.5 * (pass_color.xyz + 1.0);
    } else  // normal
    {
        float gamma = 1.0 / 2.2;
        vec3 color = pass_color.rgb;
        color = color / (color + 1.0);
        color = pow(color, vec3(gamma));
        out_color.rgb = color;
    }
    out_color.a = pass_color.a;
    // out_color.a = 1.0;
}
