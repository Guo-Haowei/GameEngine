#pragma once
#include "render_graph.h"

#define BASE_COLOR_PASS         "base_color_pass"
#define RT_RES_BASE_COLOR       "rt_res_base_color"
#define RT_RES_BASE_COLOR_DEPTH "rt_res_base_color_depth"

namespace my::rg {

void create_render_graph_base_color(RenderGraph& graph);

}  // namespace my::rg
