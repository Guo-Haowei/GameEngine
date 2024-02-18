#pragma once
#include "render_graph.h"

#define SHADOW_PASS       "shadow_pass"
#define VOXELIZATION_PASS "voxelization_pass"
#define VXGI_DEBUG_PASS   "debug_vxgi_pass"
#define GBUFFER_PASS      "gbuffer_pass"
#define LIGHTING_PASS     "lighting_pass"
#define SSAO_PASS         "ssao_pass"
#define FXAA_PASS         "fxaa_pass"
#define FINAL_PASS        "final_pass"

// @TODO: rename
#define RT_RES_SHADOW_MAP         "rt_res_shadow_map"
#define RT_RES_SSAO               "rt_res_ssao"
#define RT_RES_FXAA               "rt_res_fxaa"
#define RT_RES_LIGHTING           "rt_res_light"
#define RT_RES_GBUFFER_POSITION   "rt_res_gbuffer_position"
#define RT_RES_GBUFFER_NORMAL     "rt_res_gbuffer_normal"
#define RT_RES_GBUFFER_BASE_COLOR "rt_res_gbuffer_base_color"
#define RT_RES_GBUFFER_DEPTH      "rt_res_gbuffer_depth"
#define RT_RES_FINAL_IMAGE        "rt_res_final_image"
#define DEBUG_VXGI_OUTPUT_COLOR   "debug_vxgi_output_color"
#define DEBUG_VXGI_OUTPUT_DEPTH   "debug_vxgi_output_depth"

namespace my::rg {

void create_render_graph_vxgi(RenderGraph& graph);

}  // namespace my::rg
