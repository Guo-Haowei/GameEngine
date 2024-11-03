#ifndef CBUFFER_INCLUDED
#define CBUFFER_INCLUDED
#include "shader_defines.h"

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

using sampler2D = uint64_t;
using sampler3D = uint64_t;
using samplerCube = uint64_t;

using TextureHandle = uint64_t;

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
    mat4 projection_matrix;  // 64
    mat4 view_matrix;        // 64
    vec4 points[4];          // 64

    vec3 color;
    int type;

    vec3 position;  // direction
    int cast_shadow;

    float atten_constant;
    float atten_linear;

    float atten_quadratic;
    float max_distance;  // max distance the light affects

    vec3 padding;
    int shadow_map_index;
};

CBUFFER(PerBatchConstantBuffer, 0) {
    mat4 u_world_matrix;
    mat4 _per_batch_padding_0;
    mat4 _per_batch_padding_1;
    mat4 _per_batch_padding_2;
};

CBUFFER(PerPassConstantBuffer, 1) {
    mat4 g_view_matrix;
    mat4 g_projection_matrix;

    mat4 _per_pass_padding_0;
    mat4 _per_pass_padding_1;
};

CBUFFER(PerFrameConstantBuffer, 2) {
    Light u_lights[MAX_LIGHT_COUNT];

    int u_light_count;
    int u_display_method;
    int u_enable_bloom;
    float u_bloom_threshold;

    int u_debug_voxel_id;
    int u_no_texture;
    int u_screen_width;
    int u_screen_height;

    vec3 u_camera_position;
    float u_voxel_size;

    vec3 u_world_center;
    float u_world_size_half;

    float u_texel_size;
    int u_enable_vxgi;
    int u_debug_csm;
    int _frame_constant_padding0;
};

CBUFFER(MaterialConstantBuffer, 3) {
    vec4 u_base_color;

    float u_metallic;
    float u_roughness;
    float u_reflect_power;
    float u_emissive_power;

    int u_has_base_color_map;
    int u_has_pbr_map;
    int u_has_normal_map;
    int u_has_height_map;

    TextureHandle u_base_color_map_handle;
    TextureHandle u_normal_map_handle;

    TextureHandle u_material_map_handle;
    TextureHandle u_height_map_handle;

    vec4 _material_padding_0;
    vec4 _material_padding_1;
    vec4 _material_padding_2;
    mat4 _material_padding_3;
    mat4 _material_padding_4;
};

// @TODO: change to unordered access buffer
CBUFFER(BoneConstantBuffer, 4) {
    mat4 u_bones[MAX_BONE_COUNT];
};

// @TODO: refactor name
CBUFFER(ParticleConstantBuffer, 10) {
    int u_PreSimIdx;
    int u_PostSimIdx;
    float u_ElapsedTime;
    float u_LifeSpan;

    vec3 u_Seeds;
    float u_Scale;
    vec3 u_Position;
    int u_ParticlesPerFrame;
    vec3 u_Velocity;
    int u_MaxParticleCount;
};

#ifndef HLSL_LANG

CBUFFER(PerSceneConstantBuffer, 6) {
    // @TODO: remove the following
    sampler2D u_gbuffer_depth_map;
    sampler2D u_final_bloom;

    sampler2D u_grass_base_color;
    sampler2D c_hdr_env_map;
    sampler3D c_voxel_map;
    sampler3D c_voxel_normal_map;

    sampler2D c_kernel_noise_map;
    sampler2D c_tone_image;
    // @TODO: unordered access
    sampler2D u_ltc_1;
    sampler2D u_ltc_2;

    sampler2D c_brdf_map;
    samplerCube c_env_map;
    samplerCube c_diffuse_irradiance_map;
    samplerCube c_prefiltered_map;
};

// @TODO: make it more general, something like 2D draw
CBUFFER(DebugDrawConstantBuffer, 7) {
    vec2 c_debug_draw_pos;
    vec2 c_debug_draw_size;

    sampler2D c_debug_draw_map;
    int c_display_channel;
    int c_another_padding;
};
#endif

CBUFFER(PointShadowConstantBuffer, 8) {
    mat4 g_point_light_matrix;    // 64
    vec3 g_point_light_position;  // 12
    float g_point_light_far;      // 4

    vec4 _point_shadow_padding_0;  // 16
    vec4 _point_shadow_padding_1;  // 16
    vec4 _point_shadow_padding_2;  // 16

    mat4 _point_shadow_padding_3;  // 64
    mat4 _point_shadow_padding_4;  // 64
};

// @TODO: refactor this
CBUFFER(EnvConstantBuffer, 9) {
    mat4 g_cube_projection_view_matrix;

    float g_env_pass_roughness;  // for environment map
    float _per_env_padding_0;
    float _per_env_padding_1;
    float _per_env_padding_2;

    vec4 _per_env_padding_3;
    vec4 _per_env_padding_4;
    vec4 _per_env_padding_5;

    mat4 _per_env_padding_6;
    mat4 _per_env_padding_7;
};

#endif