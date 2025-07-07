/// File: post_process.ps.hlsl
#include "../cbuffer.hlsl.h"

uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;

#define t_TextureLighting        u_Texture0
#define t_TextureHighlightSelect u_Texture1
#define t_BloomInputTexture      u_Texture2

layout(location = 0) in vec2 pass_uv;
layout(location = 0) out vec4 out_color;

const mat3 sx = mat3(
    1.0, 2.0, 1.0,
    0.0, 0.0, 0.0,
    -1.0, -2.0, -1.0);
const mat3 sy = mat3(
    1.0, 0.0, -1.0,
    2.0, 0.0, -2.0,
    1.0, 0.0, -1.0);

#if 0
static const float3x3 sx = float3x3(
    1.0, 0.0, -1.0,
    2.0, 0.0, -2.0,
    1.0, 0.0, -1.0);
static const float3x3 sy = float3x3(
    1.0, 2.0, 1.0,
    0.0, 0.0, 0.0,
    -1.0, -2.0, -1.0);
#endif

void main() {
    vec2 uv = pass_uv;
    // flip uv
    // uv.y = 1 - uv.y;
    // Edge detection
    ivec2 dim = textureSize(t_TextureHighlightSelect, 0);
    int width = dim.x;
    int height = dim.y;
    vec2 texel_size = vec2(width, height);
    texel_size = 1.0 / texel_size;

    mat3 I;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            vec2 offset = uv + texel_size * vec2(i - 1, j - 1);
            I[i][j] = texture(t_TextureHighlightSelect, offset).r;
        }
    }

    float gx = dot(sx[0], I[0]) + dot(sx[1], I[1]) + dot(sx[2], I[2]);
    float gy = dot(sy[0], I[0]) + dot(sy[1], I[1]) + dot(sy[2], I[2]);

    float g = sqrt(pow(gx, 2.0) + pow(gy, 2.0));
    if (g > 0.0) {
        out_color = vec4(0.98, 0.64, 0.0, 1.0);
        return;
    }

    vec3 color = texture(t_TextureLighting, uv).rgb;

    // Bloom

    if (c_enableBloom == 1) {
        vec3 bloom = texture(t_BloomInputTexture, uv).rgb;
        color += bloom;
    }

    // Gamma correction
    // HDR tonemapping
    color = color / (color + vec3(1.0f, 1.0f, 1.0f));
    const float gamma = 2.2f;
    color = pow(color, 1.0f / vec3(gamma, gamma, gamma));

    out_color = vec4(color, 1.0);
}
