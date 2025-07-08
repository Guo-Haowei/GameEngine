/// File: shadow.hlsl
#include "cbuffer.hlsl.h"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

// @TODO: refactor this part
#if defined(HLSL_LANG)

#define Vector2f       float2
#define Vector3f       float3
#define Vector4f       float4
#define Multiply(a, b) mul((a), (b))
uint2 textureSizeTexture2D(Texture2D p_texture) {
    uint width, height;
    p_texture.GetDimensions(width, height);
    return uint2(width, height);
}

#elif defined(GLSL_LANG)

#define Vector2f       vec2
#define Vector3f       vec3
#define Vector4f       vec4
#define Multiply(a, b) ((a) * (b))
uvec2 textureSizeTexture2D(sampler2D textureObject) {
    return textureSize(textureObject, 0);
}

vec4 sampleShadow(Texture2D textureObject, vec2 offset) {
    return texture(textureObject, offset);
}

#else
#error "Unknown shading language"
#endif

float shadowTest(
#if defined(HLSL_LANG)
    Texture2D shadowMap,
#else
    sampler2D shadowMap,
#endif
    Light light, Vector3f worldPos, float NdotL) {
    Vector4f lightSpacePos = Multiply(light.view_matrix, Vector4f(worldPos, 1.0));
    lightSpacePos = Multiply(light.projection_matrix, lightSpacePos);
    lightSpacePos /= lightSpacePos.w;

#if defined(HLSL_LANG)
    lightSpacePos.y = -lightSpacePos.y;
    lightSpacePos.xy = 0.5 * lightSpacePos.xy + 0.5;
#else
    lightSpacePos.xyz = 0.5 * lightSpacePos.xyz + 0.5;
#endif

    float currentDepth = lightSpacePos.z;

    float shadow = 0.0;
    Vector2f texelSize = 1.0 / Vector2f(textureSizeTexture2D(shadowMap));

    // @TODO: better bias
    float bias = max(0.005 * (1.0 - NdotL), 0.0005);

    // @TODO: refactor
    for (uint SAMPLE = 0; SAMPLE < 4; ++SAMPLE) {
        uint x = SAMPLE / 2;
        uint y = SAMPLE % 2;
        Vector2f offset = Vector2f(x, y) * texelSize;
#if defined(GLSL_LANG)
        float closestDepth = texture(shadowMap, lightSpacePos.xy + offset).r;
#else
        float closestDepth = shadowMap.Sample(s_shadowSampler, lightSpacePos.xy + offset).r;
#endif
        shadow += currentDepth - bias > closestDepth ? 1.0 : 0.0;
    }

    const float samples = float(4);
    shadow /= samples;
    return shadow;
}
