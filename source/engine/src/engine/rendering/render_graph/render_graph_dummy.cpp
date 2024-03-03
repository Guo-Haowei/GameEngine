#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/rendering_dvars.h"

namespace my::rg {

void create_render_graph_dummy(RenderGraph& graph) {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int w = frame_size.x;
    int h = frame_size.y;

    create_gbuffer_pass(graph, w, h);
    graph.compile();
}

}  // namespace my::rg
