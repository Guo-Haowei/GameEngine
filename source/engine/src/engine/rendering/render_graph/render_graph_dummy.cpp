// @TODO: refactor
#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/math/frustum.h"
#include "rendering/r_cbuffers.h"
#include "rendering/render_data.h"
#include "rendering/render_graph/render_graphs.h"
#include "rendering/rendering_dvars.h"

namespace my::rg {

static void dummy_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto& graphics_manager = GraphicsManager::singleton();

    graphics_manager.set_render_target(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    // glViewport(0, 0, width, height);

    float clear_color[] = { 0.3f, 0.3f, 0.4f, 1.0f };
    graphics_manager.clear(p_subpass, CLEAR_COLOR_BIT, clear_color);
}

void create_render_graph_dummy(RenderGraph& graph) {
    ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    int w = frame_size.x;
    int h = frame_size.y;

    GraphicsManager& manager = GraphicsManager::singleton();

    auto color_attachment = manager.create_render_target(RenderTargetDesc{ RT_RES_FINAL,
                                                                           PixelFormat::R8G8B8A8_UINT,
                                                                           AttachmentType::COLOR_2D,
                                                                           w, h },
                                                         nearest_sampler());

    RenderPassDesc desc;
    desc.name = DUMMY_PASS;
    auto pass = graph.create_pass(desc);
    auto subpass = manager.create_subpass(SubpassDesc{
        .color_attachments = { color_attachment },
        .func = dummy_pass_func,
    });
    pass->add_sub_pass(subpass);

    graph.compile();
}

}  // namespace my::rg
