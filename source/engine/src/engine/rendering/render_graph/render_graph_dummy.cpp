#include "core/framework/graphics_manager.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/rendering_dvars.h"

namespace my::rg {

void create_render_graph_dummy(RenderGraph& p_graph) {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int w = frame_size.x;
    int h = frame_size.y;
    auto backend = GraphicsManager::singleton().get_backend();

    RenderPassCreator::Config config;
    config.frame_width = w;
    config.frame_height = h;
    config.enable_bloom = false;
    config.enable_ibl = false;
    config.enable_voxel_gi = false;
    config.enable_ssao = false;
    config.enable_shadow = backend == Backend::OPENGL;
    RenderPassCreator creator(config, p_graph);

    if (config.enable_shadow) {
        creator.add_shadow_pass();
    }
    creator.add_gbuffer_pass();
    creator.add_lighting_pass();

    p_graph.compile();
}

}  // namespace my::rg
