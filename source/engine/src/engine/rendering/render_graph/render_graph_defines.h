#pragma once
#include "render_graph.h"

// @TODO: rename the following
// shadow
#define SHADOW_PASS             "shadow_pass"
#define RT_RES_SHADOW_MAP       "rt_res_shadow_map"
#define RT_RES_POINT_SHADOW_MAP "rt_res_point_shadow_map"

// gbuffer
#define GBUFFER_PASS              "gbuffer_pass"
#define RT_RES_GBUFFER_POSITION   "rt_res_gbuffer_position"
#define RT_RES_GBUFFER_NORMAL     "rt_res_gbuffer_normal"
#define RT_RES_GBUFFER_BASE_COLOR "rt_res_gbuffer_base_color"
#define RT_RES_GBUFFER_MATERIAL   "rt_res_gbuffer_material"
#define RT_RES_GBUFFER_DEPTH      "rt_res_gbuffer_depth"

// vxgi pass
#define VOXELIZATION_PASS "voxelization_pass"
#define VXGI_DEBUG_PASS   "debug_vxgi_pass"
#define LIGHTING_PASS     "lighting_pass"
#define SSAO_PASS         "ssao_pass"
#define TONE_PASS         "tone_pass"
#define FINAL_PASS        "final_pass"

// select pass
#define HIGHLIGHT_SELECT_PASS   "highlight_select_pass"
#define RT_RES_HIGHLIGHT_SELECT "rt_res_highlight_selected"

// environment
#define ENV_PASS                           "env_pass"
#define RT_ENV_SKYBOX_CUBE_MAP             "env_cube_map"
#define RT_ENV_SKYBOX_DEPTH                "env_depth"
#define RT_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP "env_diffuse_irradiance_cube_map"
#define RT_ENV_DIFFUSE_IRRADIANCE_DEPTH    "env_diffuse_irradiance_depth"
#define RT_ENV_PREFILTER_CUBE_MAP          "env_prefilter_cube_map"
#define RT_ENV_PREFILTER_DEPTH             "env_prefilter_depth"
#define RT_BRDF                            "rt_brdf"

// bloom
#define BLOOM_PASS   "bloom_pass"
#define RT_RES_BLOOM "rt_res_bloom"

#define RT_RES_LIGHTING "rt_res_light"
#define RT_RES_TONE     "rt_res_tone"
#define RT_RES_SSAO     "rt_res_ssao"
#define RT_RES_FINAL    "rt_res_final"

// dummy pass
#define DUMMY_PASS "dummy_pass"

constexpr int BLOOM_MIP_CHAIN_MAX = 7;

// @TODO: move to shader_defines
constexpr int IMAGE_VOXEL_ALBEDO_SLOT = 0;
constexpr int IMAGE_VOXEL_NORMAL_SLOT = 1;
constexpr int IMAGE_BLOOM_DOWNSAMPLE_INPUT_SLOT = 2;
constexpr int IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT = 3;

namespace my::rg {

void create_render_graph_dummy(RenderGraph& p_graph);
void create_render_graph_vxgi(RenderGraph& p_graph);
// @TODO: add this
void create_render_graph_path_tracer(RenderGraph& p_graph);

}  // namespace my::rg