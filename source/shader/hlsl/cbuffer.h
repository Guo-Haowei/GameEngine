#ifndef CBUFFER_INCLUDED
#define CBUFFER_INCLUDED
#include "shader_defines.h"

// constant buffer
#if defined(__cplusplus)
#define CBUFFER(name, reg) \
    struct name : public ConstantBufferBase<name, reg>

template<typename T, int N>
struct ConstantBufferBase {
    ConstantBufferBase() {
        static_assert(sizeof(T) % 16 == 0);
    }
    constexpr int get_slot() { return N; }
    static constexpr int get_uniform_buffer_slot() { return N; }
};

using sampler2D = uint64_t;
using sampler3D = uint64_t;
using samplerCube = uint64_t;

// @TODO: remove this constraint
static_assert(MAX_CASCADE_COUNT == 4);
#elif defined(HLSL_LANG)
#define CBUFFER(name, reg) cbuffer name : register(b##reg)

#elif defined(GLSL_LANG)
#define CBUFFER(name, reg) layout(std140, binding = reg) uniform name

#endif

CBUFFER(PerBatchConstantBuffer, 0) {
    mat4 g_world;
    mat4 _per_batch_padding_0;
    mat4 _per_batch_padding_1;
    mat4 _per_batch_padding_2;
};

CBUFFER(PerPassConstantBuffer, 1) {
    mat4 g_view;
    mat4 g_projection;
    mat4 g_projection_view;

    vec3 g_point_light_position;
    float g_point_light_far;

    vec4 _per_pass_padding_0;
    vec4 _per_pass_padding_1;
    vec3 _per_pass_padding_2;
    float g_per_pass_roughness;  // for environment map
};

CBUFFER(BoneConstantBuffer, 5) {
    mat4 g_bones[MAX_BONE_COUNT];
};

#ifndef HLSL_LANG

struct Light {
    vec3 color;
    int type;
    vec3 position;  // direction
    int cast_shadow;
    samplerCube shadow_map;
    float atten_constant;
    float atten_linear;

    vec2 padding;
    float atten_quadratic;
    float max_distance;  // max distance the light affects
    mat4 matrices[6];
};

CBUFFER(PerFrameConstantBuffer, 2) {
    Light c_lights[MAX_LIGHT_COUNT];

    // @TODO: move it to Light
    mat4 c_main_light_matrices[MAX_CASCADE_COUNT];
    vec4 c_cascade_plane_distances;

    int c_light_count;
    int c_enable_csm;
    int c_display_method;
    int _c_padding_0;

    int c_debug_voxel_id;
    int c_no_texture;
    int c_screen_width;
    int c_screen_height;

    vec3 c_camera_position;
    float c_voxel_size;

    vec3 c_world_center;
    float c_world_size_half;

    int c_ssao_kernel_size;
    float c_ssao_kernel_radius;
    int c_ssao_noise_size;
    float c_texel_size;

    int c_enable_ssao;
    int c_enable_fxaa;
    int c_enable_vxgi;
    int c_debug_csm;
};

CBUFFER(MaterialConstantBuffer, 3) {
    vec4 c_albedo_color;

    float c_metallic;
    float c_roughness;
    float c_reflect_power;
    float c_emissive_power;

    int c_has_albedo_map;
    int c_has_pbr_map;
    int c_has_normal_map;
    int _c_padding1;

    sampler2D c_albedo_map;
    sampler2D c_normal_map;

    sampler2D c_pbr_map;
    sampler2D _c_dummy_padding;

    vec4 _material_padding_0;
    vec4 _material_padding_1;
    vec4 _material_padding_2;
    mat4 _material_padding_3;
    mat4 _material_padding_4;
};

CBUFFER(PerSceneConstantBuffer, 4) {
    vec4 c_ssao_kernels[MAX_SSAO_KERNEL_COUNT];

    sampler2D c_shadow_map;
    sampler2D c_hdr_env_map;
    sampler3D c_voxel_map;
    sampler3D c_voxel_normal_map;

    sampler2D g_gbuffer_base_color_map;
    sampler2D c_gbuffer_position_metallic_map;
    sampler2D c_gbuffer_normal_roughness_map;
    sampler2D g_gbuffer_material_map;

    sampler2D g_gbuffer_depth_map;
    sampler2D _some_other_padding;

    sampler2D c_ssao_map;
    sampler2D c_kernel_noise_map;
    sampler2D c_fxaa_image;
    sampler2D c_fxaa_input_image;

    sampler2D c_brdf_map;
    samplerCube c_env_map;
    samplerCube c_diffuse_irradiance_map;
    samplerCube c_prefiltered_map;
};

// @TODO: make it more general, something like 2D draw
CBUFFER(DebugDrawConstantBuffer, 6) {
    vec2 c_debug_draw_pos;
    vec2 c_debug_draw_size;

    sampler2D c_debug_draw_map;
    int c_display_channel;
    int c_another_padding;
};
#endif

#endif