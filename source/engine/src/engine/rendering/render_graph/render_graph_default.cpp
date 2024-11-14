#include "core/framework/graphics_manager.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/graphics_dvars.h"

namespace my::rg {

void CreateRenderGraphDefault(RenderGraph& p_graph) {
    const ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    const int w = frame_size.x;
    const int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frame_width = w;
    config.frame_height = h;
    config.enable_bloom = false;
    config.enable_ibl = false;
    config.enable_voxel_gi = false;
    RenderPassCreator creator(config, p_graph);

    creator.AddShadowPass();
    creator.AddGBufferPass();
    creator.AddLightingPass();
    creator.AddEmitterPass();
    creator.AddBloomPass();
    creator.AddTonePass();

    p_graph.Compile();
}

}  // namespace my::rg
