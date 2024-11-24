/// File: path_tracer.cs.glsl
#include "../shader_defines.hlsl.h"
layout(rgba16f, binding = 2) uniform image2D u_PathTracerOutputImage;

layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main() {
    ivec2 uv = ivec2(gl_GlobalInvocationID.xy);

    vec4 color = vec4(0.0, 0.5, 0.5, 1.0);
    if (uv.x > 500 && uv.y > 500) {
        color = vec4(1.0, 0.4, 0.1, 1.0);
    }

    imageStore(u_PathTracerOutputImage, uv, color);
}
