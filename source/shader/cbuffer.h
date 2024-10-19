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

using TextureHandle = uint64_t;

// @TODO: remove this constraint
#elif defined(HLSL_LANG)
#define CBUFFER(name, reg) cbuffer name : register(b##reg)

#define TextureHandle float2
#define sampler2D     float2
#define samplerCube   float2

#elif defined(GLSL_LANG)
#define CBUFFER(name, reg) layout(std140, binding = reg) uniform name

#define TextureHandle vec2

#endif

struct Light {
    mat4 projection_matrix;
    mat4 view_matrix;
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
    vec4 points[4];
};

CBUFFER(PerBatchConstantBuffer, 0) {
    mat4 u_world_matrix;
    mat4 _per_batch_padding_0;
    mat4 _per_batch_padding_1;
    mat4 _per_batch_padding_2;
};

CBUFFER(PerPassConstantBuffer, 1) {
    mat4 u_view_matrix;
    mat4 u_proj_matrix;
    mat4 u_proj_view_matrix;

    vec3 u_point_light_position;
    float u_point_light_far;

    vec4 _per_pass_padding_0;
    vec4 _per_pass_padding_1;

    sampler2D u_tmp_bloom_input;
    float u_per_pass_roughness;  // for environment map
    float _per_pass_padding_2;
};

CBUFFER(PerFrameConstantBuffer, 2) {
    Light u_lights[MAX_LIGHT_COUNT];

    // @TODO: move it to Light
    mat4 u_main_light_matrices;

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
CBUFFER(BoneConstantBuffer, 5) {
    mat4 u_bones[MAX_BONE_COUNT];
};

#ifndef HLSL_LANG

CBUFFER(PerSceneConstantBuffer, 4) {
    // @TODO: remove the following
    sampler2D u_gbuffer_depth_map;
    sampler2D u_final_bloom;

    sampler2D _dummy_padding_per_scene;
    sampler2D c_hdr_env_map;
    sampler3D c_voxel_map;
    sampler3D c_voxel_normal_map;

    sampler2D u_grass_base_color;
    sampler2D c_kernel_noise_map;
    sampler2D c_tone_image;
    sampler2D c_tone_input_image;

    sampler2D c_brdf_map;
    samplerCube c_env_map;
    samplerCube c_diffuse_irradiance_map;
    samplerCube c_prefiltered_map;

    // @TODO: unordered access
    sampler2D u_ltc_1;
    sampler2D u_ltc_2;
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