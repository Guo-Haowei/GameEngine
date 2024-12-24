/// File: cbuffer.hlsl.h
#ifndef CBUFFER_INCLUDED
#define CBUFFER_INCLUDED
#include "shader_defines.hlsl.h"

#ifdef __cplusplus
#include <engine/math/geomath.h>

#include <cstdint>
#endif

BEGIN_NAME_SPACE(my)

// constant buffer
#if defined(__cplusplus)
using TextureHandle = uint64_t;

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

struct sampler_t {
    union {
        Vector2i handle_d3d;
        uint64_t handle_gl;
    };
    sampler_t() { handle_gl = 0; }

    void Set32(int p_value) { handle_d3d.x = handle_d3d.y = p_value; }
    void Set64(uint64_t p_value) { handle_gl = p_value; }
};

static_assert(sizeof(sampler_t) == sizeof(uint64_t));

using sampler2D = sampler_t;
using sampler3D = sampler_t;
using samplerCube = sampler_t;

// @TODO: remove this constraint
#elif defined(HLSL_LANG)
#define CBUFFER(NAME, REG) cbuffer NAME : register(b##REG)

#define TextureHandle int2
#define sampler2D     int2
#define samplerCube   int2

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

    // reuse per batch buffer for bloom
    sampler2D c_BloomInputTextureResidentHandle;
    sampler2D c_BloomOutputImageResidentHandle;

    Vector2f _per_batch_padding_0;
    float c_envPassRoughness;  // for environment map
    int c_meshFlag;

    Vector4f _per_batch_padding_1;
    Vector4f _per_batch_padding_2;

    Matrix4x4f c_cubeProjectionViewMatrix;
    Matrix4x4f _per_batch_padding_5;
};

CBUFFER(PerPassConstantBuffer, 1) {
    Matrix4x4f c_viewMatrix;
    Matrix4x4f c_projectionMatrix;

    Matrix4x4f _per_pass_padding_0;
    Matrix4x4f _per_pass_padding_1;
};

CBUFFER(MaterialConstantBuffer, 2) {
    // 256
    Vector4f c_baseColor;

    float c_metallic;
    float c_roughness;
    float c_reflectPower;
    float c_emissivePower;

    int c_hasBaseColorMap;
    int c_hasMaterialMap;
    int c_hasNormalMap;
    int c_hasHeightMap;

    Vector2f c_debugDrawPos;
    Vector2f c_debugDrawSize;

    // 256
    Vector3f _material_padding_0;
    int c_displayChannel;
    Vector4f _material_padding_1;
    Vector4f _material_padding_2;
    Vector4f _material_padding_3;

    // 256
    TextureHandle c_baseColorMapHandle;
    TextureHandle c_normalMapHandle;
    TextureHandle c_materialMapHandle;
    TextureHandle c_heightMapHandle;

    sampler2D c_BaseColorMapResidentHandle;
    sampler2D c_NormalMapResidentHandle;
    sampler2D c_MaterialMapResidentHandle;
    sampler2D c_HeightMapResidentHandle;

    // 256
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

    //-----------------------------------------
    int c_lightCount;
    int c_enableBloom;
    int c_debugCsm;
    float c_bloomThreshold;  // 16

    int c_debugVoxelId;
    int c_noTexture;
    int c_enableVxgi;
    float c_texelSize;  // 16

    //-----------------------------------------

    Vector4f c_ambientColor;        // 16
    Vector4f _per_frame_padding_4;  // 16

    sampler2D c_SkyboxResidentHandle;
    sampler2D c_SkyboxHdrResidentHandle;  // 16

    uint c_DiffuseIrradianceResidentHandle;
    uint c_PrefilteredResidentHandle;
    uint c_BrdfLutResidentHandle;
    int c_forceFieldsCount;  // 16

    //-----------------------------------------

    sampler2D c_GbufferBaseColorMapResidentHandle;
    sampler2D c_GbufferPositionMapResidentHandle;  // 16

    sampler2D c_GbufferNormalMapResidentHandle;
    sampler2D c_GbufferMaterialMapResidentHandle;  // 16

    //-----------------------------------------
    sampler2D c_GbufferDepthResidentHandle;
    sampler2D c_PointShadowArrayResidentHandle;  // 16

    sampler2D c_ShadowMapResidentHandle;
    sampler2D c_TextureHighlightSelectResidentHandle;  // 16

    Vector2i c_tileOffset;
    sampler2D c_TextureLightingResidentHandle;  // 16

    Vector3f c_cameraPosition;
    float c_cameraFovDegree;  // 16

    //-----------------------------------------
    Vector3f c_voxelWorldCenter;
    float c_voxelWorldSizeHalf;  // 16

    Vector3f c_cameraForward;
    int c_frameIndex;  // 16

    Vector3f c_cameraRight;
    float c_voxelSize;

    Vector3f c_cameraUp;
    int c_sceneDirty;

    ForceField c_forceFields[MAX_FORCE_FIELD_COUNT];
};

CBUFFER(EmitterConstantBuffer, 6) {
    Vector4f c_particleColor;
    Vector3f c_seeds;
    float c_emitterScale;
    Vector3f c_emitterPosition;
    int c_particlesPerFrame;
    Vector3f c_emitterStartingVelocity;
    int c_emitterMaxParticleCount;

    int c_preSimIdx;
    int c_postSimIdx;
    float c_elapsedTime;
    float c_lifeSpan;

    Vector2i c_emitterSubUv;
    int c_emitterUseTexture;
    int c_emitterHasGravity;

    Vector3f _emitter_padding_2;
    int c_subUvCounter;

    Vector4f _emitter_padding_3;
    Matrix4x4f _emitter_padding_4;
    Matrix4x4f _emitter_padding_5;
};

#if defined(HLSL_LANG_D3D11) || defined(__cplusplus) || defined(GLSL_LANG)

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
};

#endif

END_NAME_SPACE(my)

#endif