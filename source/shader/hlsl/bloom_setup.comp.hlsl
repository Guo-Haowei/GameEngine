[numthreads(16, 16, 1)] void main(uint3 dispatch_thread_id
                                  : SV_DISPATCHTHREADID) {
}

#if 0
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../cbuffer.h"

layout(r11f_g11f_b10f, binding = 2) uniform image2D u_input_image;
layout(r11f_g11f_b10f, binding = 3) uniform image2D u_output_image;

float rgb_to_luma(vec3 rgb) {
    return sqrt(dot(rgb, vec3(0.299, 0.587, 0.114)));
}

void main() {
    ivec2 output_tex_coord = ivec2(gl_GlobalInvocationID.xy);
    vec2 output_image_size = vec2(imageSize(u_output_image));
    vec2 uv = vec2(output_tex_coord.x / output_image_size.x,
                   output_tex_coord.y / output_image_size.y);

    vec3 color = texture(g_bloom_input, vec2(uv.x, uv.y)).rgb;
    float luma = rgb_to_luma(color);
    // @TODO: dynamic lua
    if (luma < u_bloom_threshold) {
        color = vec3(0.0);
    }

    imageStore(u_output_image, output_tex_coord, vec4(color, 1.0));
}
#endif