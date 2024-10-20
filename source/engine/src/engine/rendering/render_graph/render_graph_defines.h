#pragma once

#define RENDER_PASS_NAME_LIST          \
    RENDER_PASS_NAME(SHADOW)           \
    RENDER_PASS_NAME(GBUFFER)          \
    RENDER_PASS_NAME(LIGHTING)         \
    RENDER_PASS_NAME(BLOOM)            \
    RENDER_PASS_NAME(ENV)              \
    RENDER_PASS_NAME(HIGHLIGHT_SELECT) \
    RENDER_PASS_NAME(VOXELIZATION)     \
    RENDER_PASS_NAME(TONE)             \
    RENDER_PASS_NAME(FINAL)

enum class RenderPassName {
#define RENDER_PASS_NAME(name) name,
    RENDER_PASS_NAME_LIST
#undef RENDER_PASS_NAME
        COUNT,
};

static inline const char* renderPassNameToString(RenderPassName p_name) {
    DEV_ASSERT_INDEX(p_name, RenderPassName::COUNT);

    static const char* s_names[static_cast<int>(RenderPassName::COUNT)] = {
#define RENDER_PASS_NAME(name) #name,
        RENDER_PASS_NAME_LIST
#undef RENDER_PASS_NAME
    };
    return s_names[static_cast<int>(p_name)];
}

// @TODO: rename the following
// shadow
#define RT_RES_SHADOW_MAP       "rt_res_shadow_map"
#define RT_RES_POINT_SHADOW_MAP "rt_res_point_shadow_map"

// gbuffer
#define RT_RES_GBUFFER_POSITION   "rt_res_gbuffer_position"
#define RT_RES_GBUFFER_NORMAL     "rt_res_gbuffer_normal"
#define RT_RES_GBUFFER_BASE_COLOR "rt_res_gbuffer_base_color"
#define RT_RES_GBUFFER_MATERIAL   "rt_res_gbuffer_material"
#define RT_RES_GBUFFER_DEPTH      "rt_res_gbuffer_depth"

// vxgi pass
#define VXGI_DEBUG_PASS   "debug_vxgi_pass"
#define LIGHTING_PASS     "lighting_pass"
#define TONE_PASS         "tone_pass"
#define FINAL_PASS        "final_pass"

// select pass
#define RT_RES_HIGHLIGHT_SELECT "rt_res_highlight_selected"

// environment
#define RT_ENV_SKYBOX_CUBE_MAP             "env_cube_map"
#define RT_ENV_SKYBOX_DEPTH                "env_depth"
#define RT_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP "env_diffuse_irradiance_cube_map"
#define RT_ENV_DIFFUSE_IRRADIANCE_DEPTH    "env_diffuse_irradiance_depth"
#define RT_ENV_PREFILTER_CUBE_MAP          "env_prefilter_cube_map"
#define RT_ENV_PREFILTER_DEPTH             "env_prefilter_depth"
#define RT_BRDF                            "rt_brdf"

// bloom
#define RT_RES_BLOOM "rt_res_bloom"

#define RT_RES_LIGHTING "rt_res_light"
#define RT_RES_TONE     "rt_res_tone"
#define RT_RES_FINAL    "rt_res_final"

constexpr int BLOOM_MIP_CHAIN_MAX = 7;

// @TODO: move to shader_defines
constexpr int IMAGE_VOXEL_ALBEDO_SLOT = 0;
constexpr int IMAGE_VOXEL_NORMAL_SLOT = 1;
constexpr int IMAGE_BLOOM_DOWNSAMPLE_INPUT_SLOT = 2;
constexpr int IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT = 3;

namespace my::rg {

class RenderGraph;

void createRenderGraphDefault(RenderGraph& p_graph);
void createRenderGraphVxgi(RenderGraph& p_graph);
// @TODO: add this
void createRenderGraphPathTracer(RenderGraph& p_graph);

}  // namespace my::rg