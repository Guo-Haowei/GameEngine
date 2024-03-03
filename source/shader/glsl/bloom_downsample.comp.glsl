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

    // Take 13 samples around current texel:
    // a - b - c
    // - j - k -
    // d - e - f
    // - l - m -
    // g - h - i
    // === ('e' is the current texel) ===
    vec3 a = texture(u_tmp_bloom_input, vec2(uv.x - 2 * x, uv.y + 2 * y)).rgb;
    vec3 b = texture(u_tmp_bloom_input, vec2(uv.x, uv.y + 2 * y)).rgb;
    vec3 c = texture(u_tmp_bloom_input, vec2(uv.x + 2 * x, uv.y + 2 * y)).rgb;

    vec3 d = texture(u_tmp_bloom_input, vec2(uv.x - 2 * x, uv.y)).rgb;
    vec3 e = texture(u_tmp_bloom_input, vec2(uv.x, uv.y)).rgb;
    vec3 f = texture(u_tmp_bloom_input, vec2(uv.x + 2 * x, uv.y)).rgb;

    vec3 g = texture(u_tmp_bloom_input, vec2(uv.x - 2 * x, uv.y - 2 * y)).rgb;
    vec3 h = texture(u_tmp_bloom_input, vec2(uv.x, uv.y - 2 * y)).rgb;
    vec3 i = texture(u_tmp_bloom_input, vec2(uv.x + 2 * x, uv.y - 2 * y)).rgb;

    vec3 j = texture(u_tmp_bloom_input, vec2(uv.x - x, uv.y + y)).rgb;
    vec3 k = texture(u_tmp_bloom_input, vec2(uv.x + x, uv.y + y)).rgb;
    vec3 l = texture(u_tmp_bloom_input, vec2(uv.x - x, uv.y - y)).rgb;
    vec3 m = texture(u_tmp_bloom_input, vec2(uv.x + x, uv.y - y)).rgb;

    // This shows 5 square areas that are being sampled. But some of them overlap,
    // so to have an energy preserving downsample we need to make some adjustments.
    // The weights are the distributed, so that the sum of j,k,l,m (e.g.)
    // contribute 0.5 to the final color output. The code below is written
    // to effectively yield this sum. We get:
    // 0.125*5 + 0.03125*4 + 0.0625*4 = 1

    // Apply weighted distribution:
    // 0.5 + 0.125 + 0.125 + 0.125 + 0.125 = 1
    // a,b,d,e * 0.125
    // b,c,e,f * 0.125
    // d,e,g,h * 0.125
    // e,f,h,i * 0.125
    // j,k,l,m * 0.5
    vec3 final_color = e * 0.125;
    final_color += (a + c + g + i) * 0.03125;
    final_color += (b + d + f + h) * 0.0625;
    final_color += (j + k + l + m) * 0.125;

    final_color = max(final_color, 0.000f);
    // final_color = max(final_color, 0.0001f);
    imageStore(u_output_image, output_tex_coord, vec4(final_color, 1.0));
}