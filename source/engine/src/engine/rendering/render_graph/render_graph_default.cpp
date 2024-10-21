#include "core/framework/graphics_manager.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/rendering_dvars.h"

namespace my::rg {

void createRenderGraphDefault(RenderGraph& p_graph) {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int w = frame_size.x;
    int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frame_width = w;
    config.frame_height = h;
    config.enable_bloom = false;
    config.enable_ibl = false;
    config.enable_voxel_gi = false;
    RenderPassCreator creator(config, p_graph);

    creator.addShadowPass();
    creator.addGBufferPass();
    creator.addLightingPass();
    creator.addTonePass(true);

    p_graph.compile();
}

}  // namespace my::rg
