#include "texture_binding.h"

// @TODO: refactor this part
#if defined(HLSL_LANG)

#define Vector2f       float2
#define Vector3f       float3
#define Vector4f       float4
#define Multiply(a, b) mul((a), (b))
int2 textureSizeTexture2D(Texture2D p_texture) {
    int width, height;
    p_texture.GetDimensions(width, height);
    return int2(width, height);
}
SamplerState g_shadow_sampler : register(s1);

float4 sampleShadow(Texture2D textureObject, float2 offset) {
    return textureObject.Sample(g_shadow_sampler, offset);
}

#elif defined(GLSL_LANG)

#define Vector2f       vec2
#define Vector3f       vec3
#define Vector4f       vec4
#define Multiply(a, b) ((a) * (b))
ivec2 textureSizeTexture2D(Texture2D textureObject) {
    return textureSize(textureObject, 0);
}

vec4 sampleShadow(Texture2D textureObject, vec2 offset) {
    return texture(textureObject, offset);
}

#else
#error "Unknown shading language"
#endif

float shadowTest(Texture2D shadowMap, Vector3f worldPos, float NdotL, int quality) {
    Vector4f lightSpacePos = Multiply(u_main_light_matrices, Vector4f(worldPos, 1.0));
    Vector3f coords = lightSpacePos.xyz / lightSpacePos.w;
    coords = 0.5 * coords + 0.5;
    float currentDepth = coords.z;

    float shadow = 0.0;
    Vector2f texelSize = 1.0 / Vector2f(textureSizeTexture2D(shadowMap));

    // @TODO: better bias
    float bias = max(0.005 * (1.0 - NdotL), 0.0005);

    for (int x = -quality; x <= quality; ++x) {
        for (int y = -quality; y <= quality; ++y) {
            Vector2f offset = Vector2f(x, y) * texelSize;
            float closestDepth = sampleShadow(shadowMap, coords.xy + offset).r;
            shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
        }
    }

    const float samples = float(2 * quality + 1);
    shadow /= samples * samples;
    return shadow;
}
