layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

#include "../cbuffer.h"

layout(r11f_g11f_b10f, binding = 3) uniform image2D u_output_image;

void main() {
    ivec2 output_tex_coord = ivec2(gl_GlobalInvocationID.xy);
    vec2 output_image_size = vec2(imageSize(u_output_image));
    vec2 uv = vec2(float(output_tex_coord.x) / output_image_size.x,
                   float(output_tex_coord.y) / output_image_size.y);

    vec2 input_tex_size = textureSize(u_tmp_bloom_input, 0);
    vec2 input_texel_size = 1.0 / input_tex_size;
    float x = input_texel_size.x;
    float y = input_texel_size.y;
    uv.x += 0.5 * x;
    uv.y += 0.5 * y;

    float filterRadius = 0.005;
    x = filterRadius;
    y = filterRadius;

    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(u_tmp_bloom_input, vec2(uv.x - x, uv.y + y)).rgb;
    vec3 b = texture(u_tmp_bloom_input, vec2(uv.x, uv.y + y)).rgb;
    vec3 c = texture(u_tmp_bloom_input, vec2(uv.x + x, uv.y + y)).rgb;

    vec3 d = texture(u_tmp_bloom_input, vec2(uv.x - x, uv.y)).rgb;
    vec3 e = texture(u_tmp_bloom_input, vec2(uv.x, uv.y)).rgb;
    vec3 f = texture(u_tmp_bloom_input, vec2(uv.x + x, uv.y)).rgb;

    vec3 g = texture(u_tmp_bloom_input, vec2(uv.x - x, uv.y - y)).rgb;
    vec3 h = texture(u_tmp_bloom_input, vec2(uv.x, uv.y - y)).rgb;
    vec3 i = texture(u_tmp_bloom_input, vec2(uv.x + x, uv.y - y)).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    vec3 upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += (a + c + g + i);
    upsample *= 1.0 / 16.0;

    vec3 final_color = imageLoad(u_output_image, output_tex_coord).rgb;
    final_color = mix(final_color, upsample, 0.6);
    imageStore(u_output_image, output_tex_coord, vec4(final_color, 1.0));
}
