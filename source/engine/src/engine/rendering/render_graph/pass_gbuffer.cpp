#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_data.h"
#include "rendering/render_graph/pass_creator.h"

namespace my::rg {

static void gbuffer_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto& graphics_manager = GraphicsManager::singleton();
    auto [width, height] = p_subpass->depth_attachment->get_size();

    graphics_manager.set_render_target(p_subpass);

    Viewport viewport{ width, height };
    graphics_manager.set_viewport(viewport);

    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    graphics_manager.clear(p_subpass, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT, clear_color);

    auto render_data = graphics_manager.get_render_data();
    RenderData::Pass& pass = render_data->main_pass;

    pass.fill_perpass(g_per_pass_cache.cache);
    g_per_pass_cache.update();

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            graphics_manager.uniform_bind_slot<BoneConstantBuffer>(render_data->m_bone_uniform.get(), draw.bone_idx);
        }

        graphics_manager.set_pipeline_state(has_bone ? PROGRAM_GBUFFER_ANIMATED : PROGRAM_GBUFFER_STATIC);

        if (draw.flags) {
            graphics_manager.set_stencil_ref(draw.flags);
        }

        graphics_manager.uniform_bind_slot<PerBatchConstantBuffer>(render_data->m_batch_uniform.get(), draw.batch_idx);

        graphics_manager.set_mesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            const MaterialConstantBuffer& material = render_data->m_material_buffers[subset.material_idx];
            graphics_manager.bind_texture(Dimension::TEXTURE_2D, material.u_base_color_map_handle, u_base_color_map_slot);
            graphics_manager.bind_texture(Dimension::TEXTURE_2D, material.u_normal_map_handle, u_normal_map_slot);
            graphics_manager.bind_texture(Dimension::TEXTURE_2D, material.u_material_map_handle, u_material_map_slot);

            graphics_manager.uniform_bind_slot<MaterialConstantBuffer>(render_data->m_material_uniform.get(), subset.material_idx);

            // @TODO: set material

            graphics_manager.draw_elements(subset.index_count, subset.index_offset);
        }

        if (draw.flags) {
            graphics_manager.set_stencil_ref(0);
        }
    }
}

void RenderPassCreator::add_gbuffer_pass() {
    GraphicsManager& manager = GraphicsManager::singleton();

    int p_width = m_config.frame_width;
    int p_height = m_config.frame_height;

    // @TODO: decouple sampler and render target
    auto gbuffer_depth = manager.create_render_target(RenderTargetDesc{ RT_RES_GBUFFER_DEPTH,
                                                                        PixelFormat::D24_UNORM_S8_UINT,
                                                                        AttachmentType::DEPTH_STENCIL_2D,
                                                                        p_width, p_height },
                                                      nearest_sampler());

    auto attachment0 = manager.create_render_target(RenderTargetDesc{ RT_RES_GBUFFER_BASE_COLOR,
                                                                      PixelFormat::R11G11B10_FLOAT,
                                                                      AttachmentType::COLOR_2D,
                                                                      p_width, p_height },
                                                    nearest_sampler());

    auto attachment1 = manager.create_render_target(RenderTargetDesc{ RT_RES_GBUFFER_POSITION,
                                                                      PixelFormat::R16G16B16_FLOAT,
                                                                      AttachmentType::COLOR_2D,
                                                                      p_width, p_height },
                                                    nearest_sampler());

    auto attachment2 = manager.create_render_target(RenderTargetDesc{ RT_RES_GBUFFER_NORMAL,
                                                                      PixelFormat::R16G16B16_FLOAT,
                                                                      AttachmentType::COLOR_2D,
                                                                      p_width, p_height },
                                                    nearest_sampler());

    auto attachment3 = manager.create_render_target(RenderTargetDesc{ RT_RES_GBUFFER_MATERIAL,
                                                                      PixelFormat::R11G11B10_FLOAT,
                                                                      AttachmentType::COLOR_2D,
                                                                      p_width, p_height },
                                                    nearest_sampler());

    RenderPassDesc desc;
    desc.name = GBUFFER_PASS;
    auto pass = m_graph.create_pass(desc);
    auto subpass = manager.create_subpass(SubpassDesc{
        .color_attachments = { attachment0, attachment1, attachment2, attachment3 },
        .depth_attachment = gbuffer_depth,
        .func = gbuffer_pass_func,
    });
    pass->add_sub_pass(subpass);
}

}  // namespace my::rg
