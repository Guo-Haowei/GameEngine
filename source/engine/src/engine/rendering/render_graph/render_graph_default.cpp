#include "render_graph_default.h"

// @TODO: refactor
#include "core/base/rid_owner.h"
#include "core/framework/graphics_manager.h"
#include "core/framework/scene_manager.h"
#include "core/math/frustum.h"
#include "rendering/r_cbuffers.h"
#include "rendering/render_data.h"
#include "rendering/rendering_dvars.h"
#include "servers/display_server.h"

extern my::RIDAllocator<MeshData> g_meshes;

namespace my::rg {

void create_render_graph_default(RenderGraph& graph) {
    unused(graph);
}

}  // namespace my::rg
