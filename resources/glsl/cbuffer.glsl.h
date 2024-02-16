#ifndef CBUFFER_INCLUDED
#define CBUFFER_INCLUDED

#define SHADER_LIGHT_MAX       16
#define SHADER_BONE_MAX        128
#define SHADER_SSAO_KERNEL_MAX 64

// @TODO: rename
#define NUM_CASCADES 3

#define MAX_MATERIALS  300
#define MAX_LIGHT_ICON 4

// constant buffer
#ifdef __cplusplus
#define CONSTANT_BUFFER(name, reg) \
    struct name : public ConstantBufferBase<reg>
template<int N>
struct ConstantBufferBase {
    constexpr int get_slot() { return N; }
};
#else
#define CONSTANT_BUFFER(name, reg) layout(std140, binding = reg) uniform name
#endif

// sampler
#ifdef __cplusplus
using sampler2D = uint64_t;
using sampler3D = uint64_t;
typedef struct {
    uint64_t data;
    uint64_t padding;
} Sampler2DArray;
#else
#define Sampler2DArray sampler2D
#endif

struct Light {
    vec3 color;
    int type;
    vec3 position;  // direction
    int cast_shadow;
    float atten_constant;
    float atten_linear;
    float atten_quadratic;
    float padding;
    mat4 light_matricies[NUM_CASCADES];
    // @TODO: shadow map
};

CONSTANT_BUFFER(PerFrameConstantBuffer, 0) {
    mat4 c_view_matrix;
    mat4 c_projection_matrix;
    mat4 c_projection_view_matrix;

    Light c_lights[SHADER_LIGHT_MAX];

    vec3 _c_padding_0;
    int c_light_count;

    vec3 c_camera_position;
    float c_voxel_size;

    vec4 c_cascade_clip_z;

    vec3 c_world_center;
    float c_world_size_half;

    int c_debug_texture_id;
    int c_no_texture;
    int c_screen_width;
    int c_screen_height;

    int c_ssao_kernel_size;
    float c_ssao_kernel_radius;
    int c_ssao_noise_size;
    float c_texel_size;

    int c_enable_ssao;
    int c_enable_fxaa;
    int c_enable_vxgi;
    int c_debug_csm;
};

CONSTANT_BUFFER(PerBatchConstantBuffer, 1) {
    mat4 c_projection_view_model_matrix;
    mat4 c_model_matrix;
};

CONSTANT_BUFFER(MaterialConstantBuffer, 2) {
    vec4 c_albedo_color;
    float c_metallic;
    float c_roughness;
    int c_has_albedo_map;
    int c_has_pbr_map;

    int c_has_normal_map;
    int c_texture_map_idx;
    float c_reflect_power;
    int _c_padding1;
};

CONSTANT_BUFFER(PerSceneConstantBuffer, 3) {
    vec4 c_ssao_kernels[SHADER_SSAO_KERNEL_MAX];
    sampler2D c_shadow_map;
    sampler3D c_voxel_map;
    sampler3D c_voxel_normal_map;
    sampler3D c_voxel_emissive_map;
    sampler2D c_gbuffer_albedo_map;
    sampler2D c_gbuffer_position_metallic_map;
    sampler2D c_gbuffer_normal_roughness_map;
    sampler2D c_gbuffer_depth_map;
    sampler2D c_ssao_map;
    sampler2D c_kernel_noise_map;
    sampler2D c_fxaa_image;
    sampler2D c_fxaa_input_image;
    Sampler2DArray c_albedo_maps[MAX_MATERIALS];
    Sampler2DArray c_normal_maps[MAX_MATERIALS];
    Sampler2DArray c_pbr_maps[MAX_MATERIALS];
};

CONSTANT_BUFFER(BoneConstantBuffer, 4) {
    mat4 c_bones[SHADER_BONE_MAX];
};

#endif