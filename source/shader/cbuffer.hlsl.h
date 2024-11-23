/// File: cbuffer.hlsl.h
#ifndef CBUFFER_INCLUDED
#define CBUFFER_INCLUDED
#include "shader_defines.hlsl.h"

// constant buffer
#if defined(__cplusplus)

template<typename T, int N>
struct ConstantBufferBase {
    ConstantBufferBase() {
        static_assert(sizeof(T) % 16 == 0);
    }
    constexpr int GetSlot() { return N; }
    static constexpr int GetUniformBufferSlot() { return N; }
};

#define CBUFFER(NAME, REG) \
    struct NAME : public ConstantBufferBase<NAME, REG>

// @TODO: remove this constraint
#elif defined(HLSL_LANG)
#define CBUFFER(NAME, REG) cbuffer NAME : register(b##REG)

#define TextureHandle float2
#define sampler2D     float2
#define samplerCube   float2

#elif defined(GLSL_LANG)
#define CBUFFER(NAME, REG) layout(std140, binding = REG) uniform NAME

#define TextureHandle vec2

#endif

struct Light {
    Matrix4x4f projection_matrix;  // 64
    Matrix4x4f view_matrix;        // 64
    Vector4f points[4];            // 64

    Vector3f color;
    int type;

    Vector3f position;  // direction
    int cast_shadow;

    float atten_constant;
    float atten_linear;

    float atten_quadratic;
    float max_distance;  // max distance the light affects

    Vector3f padding;
    int shadow_map_index;
};

struct ForceField {
    Vector3f position;
    float strength;
};

CBUFFER(PerBatchConstantBuffer, 0) {
    Matrix4x4f c_worldMatrix;

    Vector3f _per_batch_padding_0;
    int c_hasAnimation;

    Vector4f _per_batch_padding_1;

    Vector4f _per_batch_padding_2;

    // reuse per batch buffer for bloom
    Vector2f _per_batch_padding_3;
    uint c_BloomInputTextureIndex;
    uint c_BloomOutputImageIndex;

    Matrix4x4f _per_batch_padding_4;
    Matrix4x4f _per_batch_padding_5;
};

CBUFFER(PerPassConstantBuffer, 1) {
    Matrix4x4f c_viewMatrix;
    Matrix4x4f c_projectionMatrix;

    Matrix4x4f _per_pass_padding_0;
    Matrix4x4f _per_pass_padding_1;
};

CBUFFER(MaterialConstantBuffer, 2) {
    Vector4f c_baseColor;

    float c_metallic;
    float c_roughness;
    float c_reflectPower;
    float c_emissivePower;

    int c_hasBaseColorMap;
    int c_hasMaterialMap;
    int c_hasNormalMap;
    int c_hasHeightMap;

    uint c_BaseColorMapIndex;
    uint c_NormalMapIndex;
    uint c_MaterialMapIndex;
    uint c_HeightMapIndex;

    TextureHandle c_baseColorMapHandle;
    TextureHandle c_normalMapHandle;
    TextureHandle c_materialMapHandle;
    TextureHandle c_heightMapHandle;

    Vector4f _material_padding_1;
    Vector4f _material_padding_2;
    Matrix4x4f _material_padding_3;
    Matrix4x4f _material_padding_4;
};

// @TODO: change to unordered access buffer
CBUFFER(BoneConstantBuffer, 3) {
    Matrix4x4f c_bones[MAX_BONE_COUNT];
};

CBUFFER(PointShadowConstantBuffer, 4) {
    Matrix4x4f c_pointLightMatrix;  // 64
    Vector3f c_pointLightPosition;  // 12
    float c_pointLightFar;          // 4

    Vector4f _point_shadow_padding_0;  // 16
    Vector4f _point_shadow_padding_1;  // 16
    Vector4f _point_shadow_padding_2;  // 16

    Matrix4x4f _point_shadow_padding_3;  // 64
    Matrix4x4f _point_shadow_padding_4;  // 64
};

CBUFFER(PerFrameConstantBuffer, 5) {
    Light c_lights[MAX_LIGHT_COUNT];

    int c_lightCount;
    int c_enableBloom;
    int c_debugCsm;
    float c_bloomThreshold;  // 16

    int c_debugVoxelId;
    int c_noTexture;
    int c_enableVxgi;
    float c_texelSize;  // 16

    Vector3f c_cameraPosition;
    float c_voxelSize;  // 16

    Vector3f c_worldCenter;
    float c_worldSizeHalf;  // 16

    uint c_GbufferBaseColorMapIndex;
    uint c_GbufferPositionMapIndex;
    uint c_GbufferNormalMapIndex;
    uint c_GbufferMaterialMapIndex;  // 16

    uint c_GbufferDepthIndex;
    uint c_PointShadowArrayIndex;
    uint c_ShadowMapIndex;
    uint c_TextureHighlightSelectIndex;

    Vector2f _per_frame_padding_1;
    uint c_TextureLightingIndex;
    int c_forceFieldsCount;

    Vector4f _per_frame_padding_2;  // 16

    Matrix4x4f _per_frame_padding_3;  // 64
    Matrix4x4f _per_frame_padding_4;  // 64

    ForceField c_forceFields[MAX_FORCE_FIELD_COUNT];
};

CBUFFER(EmitterConstantBuffer, 6) {
    int c_preSimIdx;
    int c_postSimIdx;
    float c_elapsedTime;
    float c_lifeSpan;

    Vector3f c_seeds;
    float c_emitterScale;
    Vector3f c_emitterPosition;
    int c_particlesPerFrame;
    Vector3f c_emitterStartingVelocity;
    int c_emitterMaxParticleCount;

    Vector3f _emitter_padding_0;
    int c_emitterHasGravity;

    Vector4f _emitter_padding_1;
    Vector4f _emitter_padding_2;
    Vector4f _emitter_padding_3;
    Matrix4x4f _emitter_padding_4;
    Matrix4x4f _emitter_padding_5;
};

#if defined(HLSL_LANG_D3D11) || defined(__cplusplus) || defined(GLSL_LANG)

#if defined(GLSL_LANG) || defined(__cplusplus)

// @TODO: merge it with per frame
CBUFFER(PerSceneConstantBuffer, 7) {
    // @TODO: remove the following
    sampler2D _per_scene_padding_0;
    sampler2D c_finalBloom;

    sampler2D c_grassBaseColor;
    sampler2D c_hdrEnvMap;

    sampler2D c_kernelNoiseMap;
    sampler2D c_toneImage;
    // @TODO: unordered access
    sampler2D c_ltc1;
    sampler2D c_ltc2;

    sampler2D c_brdfMap;
    samplerCube c_envMap;
    samplerCube c_diffuseIrradianceMap;
    samplerCube c_prefilteredMap;
};

// @TODO: make it more general, something like 2D draw
CBUFFER(DebugDrawConstantBuffer, 8) {
    Vector2f c_debugDrawPos;
    Vector2f c_debugDrawSize;

    sampler2D c_debugDrawMap;
    int c_displayChannel;
    int _debug_draw_padding_0;
};
#endif

// @TODO: refactor this
CBUFFER(EnvConstantBuffer, 9) {
    Matrix4x4f c_cubeProjectionViewMatrix;

    float c_envPassRoughness;  // for environment map
    float _env_padding_0;
    float _env_padding_1;
    float _env_padding_2;

    Vector4f _env_padding_3;
    Vector4f _env_padding_4;
    Vector4f _env_padding_5;

    Matrix4x4f _env_padding_6;
    Matrix4x4f _env_padding_7;
};

#endif

#endif