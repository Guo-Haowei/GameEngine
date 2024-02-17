#pragma once
#include "render_graph.h"

namespace my::rg {

void shadow_pass_func();
void gbuffer_pass_func();
void voxelization_pass_func();
void debug_vxgi_pass_func();
void ssao_pass_func();
void lighting_pass_func();
void fxaa_pass_func();
void final_pass_func();

void create_render_graph_default(RenderGraph& graph);

}  // namespace my::rg
