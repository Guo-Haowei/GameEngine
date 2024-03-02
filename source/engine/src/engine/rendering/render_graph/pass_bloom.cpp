#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_data.h"
#include "rendering/render_graph/render_graph_defines.h"

// @TODO: remove API sepcific code
#include "drivers/opengl/opengl_prerequisites.h"

namespace my::rg {

static void down_sample_func(const Subpass* p_subpass, int p_level) {
    unused(p_subpass);
    unused(p_level);
}

void create_bloom_pass(RenderGraph& p_graph, int p_width, int p_height) {
    GraphicsManager& manager = GraphicsManager::singleton();

    RenderPassDesc desc;
    desc.name = BLOOM_PASS;
    desc.dependencies = { LIGHTING_PASS };
    auto pass = p_graph.create_pass(desc);

    // @TODO: refactor
    SubPassFunc funcs[] = {
        [](const Subpass* p_subpass) {
            down_sample_func(p_subpass, 0);
        },
        [](const Subpass* p_subpass) {
            down_sample_func(p_subpass, 1);
        },
        [](const Subpass* p_subpass) {
            down_sample_func(p_subpass, 2);
        },
        [](const Subpass* p_subpass) {
            down_sample_func(p_subpass, 3);
        },
        [](const Subpass* p_subpass) {
            down_sample_func(p_subpass, 4);
        },
        [](const Subpass* p_subpass) {
            down_sample_func(p_subpass, 5);
        },
    };
    static_assert(array_length(funcs) == BLOOM_MIP_CHAIN_MAX);

    for (int i = 0, width = p_width / 2, height = p_height / 2; i < BLOOM_MIP_CHAIN_MAX; ++i, width /= 2, height /= 2) {
        DEV_ASSERT(width > 1);
        DEV_ASSERT(height > 1);

        LOG_WARN("bloom size {}x{}", width, height);

        auto attachment = manager.create_render_target(RenderTargetDesc{ std::format("{}_{}", RT_RES_BLOOM, i),
                                                                         PixelFormat::R11G11B10_FLOAT,
                                                                         AttachmentType::COLOR_2D,
                                                                         width, height },
                                                       linear_clamp_sampler());
        auto subpass = manager.create_subpass(SubpassDesc{
            .color_attachments = { attachment },
            .func = funcs[i],
        });
        pass->add_sub_pass(subpass);
    }
}

}  // namespace my::rg
