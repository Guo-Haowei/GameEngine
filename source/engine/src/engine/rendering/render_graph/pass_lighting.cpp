#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_data.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/render_manager.h"

// @TODO: refactor
#include "core/framework/scene_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"

extern void fill_camera_matrices(PerPassConstantBuffer& buffer);

namespace my::rg {

static void lighting_pass_func(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto& manager = GraphicsManager::singleton();
    manager.set_render_target(p_subpass);
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto [width, height] = p_subpass->color_attachments[0]->get_size();

    Viewport viewport(width, height);
    manager.set_viewport(viewport);
    manager.clear(p_subpass, CLEAR_COLOR_BIT);
    manager.set_pipeline_state(PROGRAM_LIGHTING);

    // @TODO: refactor pass to auto bind resources,
    // and make it a class so don't do a map search every frame
    auto bind_slot = [&](const std::string& name, int slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<RenderTarget> resource = manager.find_render_target(name);
        if (!resource) {
            return;
        }

        manager.bind_texture(p_dimension, resource->texture->get_handle(), slot);
    };

    // bind common textures
    bind_slot(RT_RES_GBUFFER_BASE_COLOR, u_gbuffer_base_color_map_slot);
    bind_slot(RT_RES_GBUFFER_POSITION, u_gbuffer_position_map_slot);
    bind_slot(RT_RES_GBUFFER_NORMAL, u_gbuffer_normal_map_slot);
    bind_slot(RT_RES_GBUFFER_MATERIAL, u_gbuffer_material_map_slot);

    // @TODO: fix it
    RenderManager::singleton().draw_quad();

    // @TODO: fix it
    auto camera = SceneManager::singleton().get_scene().m_camera;
    fill_camera_matrices(g_per_pass_cache.cache);
    g_per_pass_cache.update();

    // if (0) {
    //     // draw billboard grass here for now
    //     manager.set_pipeline_state(PROGRAM_BILLBOARD);
    //     manager.set_mesh(&g_grass);
    //     glDrawElementsInstanced(GL_TRIANGLES, g_grass.index_count, GL_UNSIGNED_INT, 0, 64);
    // }

    // @TODO: fix skybox
    if (GraphicsManager::singleton().get_backend() == Backend::OPENGL) {
        auto render_data = GraphicsManager::singleton().get_render_data();
        RenderData::Pass& pass = render_data->main_pass;

        pass.fill_perpass(g_per_pass_cache.cache);
        g_per_pass_cache.update();
        GraphicsManager::singleton().set_pipeline_state(PROGRAM_ENV_SKYBOX);
        RenderManager::singleton().draw_skybox();
    }
}

void RenderPassCreator::add_lighting_pass() {
    GraphicsManager& manager = GraphicsManager::singleton();

    auto gbuffer_depth = manager.find_render_target(RT_RES_GBUFFER_DEPTH);

    auto lighting_attachment = manager.create_render_target(RenderTargetDesc{ RT_RES_LIGHTING,
                                                                              PixelFormat::R11G11B10_FLOAT,
                                                                              AttachmentType::COLOR_2D,
                                                                              m_config.frame_width, m_config.frame_height },
                                                            nearest_sampler());

    RenderPassDesc desc;
    desc.name = LIGHTING_PASS;

    desc.dependencies = { GBUFFER_PASS };
    if (m_config.enable_shadow) {
        desc.dependencies.push_back(SHADOW_PASS);
    }
    if (m_config.enable_voxel_gi) {
        desc.dependencies.push_back(VOXELIZATION_PASS);
    }
    if (m_config.enable_ibl) {
        desc.dependencies.push_back(ENV_PASS);
    }

    auto pass = m_graph.create_pass(desc);
    auto subpass = manager.create_subpass(SubpassDesc{
        .color_attachments = { lighting_attachment },
        .depth_attachment = gbuffer_depth,
        .func = lighting_pass_func,
    });
    pass->add_sub_pass(subpass);
}

}  // namespace my::rg
