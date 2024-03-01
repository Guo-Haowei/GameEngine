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

// base_color pass
#define BASE_COLOR_PASS         "base_color_pass"
#define RT_RES_BASE_COLOR       "rt_res_base_color"
#define RT_RES_BASE_COLOR_DEPTH "rt_res_base_color_depth"

// vxgi pass
#define VOXELIZATION_PASS "voxelization_pass"
#define VXGI_DEBUG_PASS   "debug_vxgi_pass"
#define LIGHTING_PASS     "lighting_pass"
#define SSAO_PASS         "ssao_pass"
#define FXAA_PASS         "fxaa_pass"
#define FINAL_PASS        "final_pass"

// environment
#define ENV_PASS                           "env_pass"
#define RT_ENV_SKYBOX_CUBE_MAP             "env_cube_map"
#define RT_ENV_SKYBOX_DEPTH                "env_depth"
#define RT_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP "env_diffuse_irradiance_cube_map"
#define RT_ENV_DIFFUSE_IRRADIANCE_DEPTH    "env_diffuse_irradiance_depth"
#define RT_ENV_PREFILTER_CUBE_MAP          "env_prefilter_cube_map"
#define RT_ENV_PREFILTER_DEPTH             "env_prefilter_depth"
#define RT_BRDF                            "rt_brdf"

#define RT_RES_LIGHTING "rt_res_light"
#define RT_RES_FXAA     "rt_res_fxaa"
#define RT_RES_SSAO     "rt_res_ssao"
#define RT_RES_FINAL    "rt_res_final"

// dummy pass
#define DUMMY_PASS "dummy_pass"

namespace my::rg {

void create_render_graph_dummy(RenderGraph& graph);
void create_render_graph_base_color(RenderGraph& graph);
void create_render_graph_vxgi(RenderGraph& graph);

}  // namespace my::rg