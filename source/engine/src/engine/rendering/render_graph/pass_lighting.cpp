#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_data.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/render_manager.h"

// @TODO: refactor
#include "core/framework/scene_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"

extern void fill_camera_matrices(PerPassConstantBuffer& p_buffer);

namespace my::rg {

static void lightingPassFunc(const Subpass* p_subpass) {
    OPTICK_EVENT();

    auto& manager = GraphicsManager::singleton();
    DEV_ASSERT(!p_subpass->color_attachments.empty());
    auto [width, height] = p_subpass->color_attachments[0]->getSize();

    manager.setRenderTarget(p_subpass);

    Viewport viewport(width, height);
    manager.setViewport(viewport);
    manager.clear(p_subpass, CLEAR_COLOR_BIT);
    manager.setPipelineState(PROGRAM_LIGHTING);

    // @TODO: refactor pass to auto bind resources,
    // and make it a class so don't do a map search every frame
    auto bind_slot = [&](const std::string& name, int slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<RenderTarget> resource = manager.findRenderTarget(name);
        if (!resource) {
            return;
        }

        manager.bindTexture(p_dimension, resource->texture->get_handle(), slot);
    };

    // bind common textures
    bind_slot(RT_RES_GBUFFER_BASE_COLOR, u_gbuffer_base_color_map_slot);
    bind_slot(RT_RES_GBUFFER_POSITION, u_gbuffer_position_map_slot);
    bind_slot(RT_RES_GBUFFER_NORMAL, u_gbuffer_normal_map_slot);
    bind_slot(RT_RES_GBUFFER_MATERIAL, u_gbuffer_material_map_slot);
    bind_slot(RT_RES_SHADOW_MAP, u_shadow_map_slot);

    // @TODO: fix it
    RenderManager::singleton().draw_quad();

    // @TODO: fix it
    auto camera = SceneManager::singleton().getScene().m_camera;
    fill_camera_matrices(g_per_pass_cache.cache);
    g_per_pass_cache.update();

    // if (0) {
    //     // draw billboard grass here for now
    //     manager.setPipelineState(PROGRAM_BILLBOARD);
    //     manager.setMesh(&g_grass);
    //     glDrawElementsInstanced(GL_TRIANGLES, g_grass.index_count, GL_UNSIGNED_INT, 0, 64);
    // }

    // @TODO: fix skybox
    if (GraphicsManager::singleton().getBackend() == Backend::OPENGL) {
        auto render_data = GraphicsManager::singleton().getRenderData();
        RenderData::Pass& pass = render_data->main_pass;

        pass.fillPerpass(g_per_pass_cache.cache);
        g_per_pass_cache.update();
        GraphicsManager::singleton().setPipelineState(PROGRAM_ENV_SKYBOX);
        RenderManager::singleton().draw_skybox();
    }

    // unbind stuff
    manager.unbindTexture(Dimension::TEXTURE_2D, u_gbuffer_base_color_map_slot);
    manager.unbindTexture(Dimension::TEXTURE_2D, u_gbuffer_position_map_slot);
    manager.unbindTexture(Dimension::TEXTURE_2D, u_gbuffer_normal_map_slot);
    manager.unbindTexture(Dimension::TEXTURE_2D, u_gbuffer_material_map_slot);
}

void RenderPassCreator::addLightingPass() {
    GraphicsManager& manager = GraphicsManager::singleton();

    auto gbuffer_depth = manager.findRenderTarget(RT_RES_GBUFFER_DEPTH);

    auto lighting_attachment = manager.createRenderTarget(RenderTargetDesc{ RT_RES_LIGHTING,
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

    auto pass = m_graph.createPass(desc);
    auto subpass = manager.createSubpass(SubpassDesc{
        .color_attachments = { lighting_attachment },
        .depth_attachment = gbuffer_depth,
        .func = lightingPassFunc,
    });
    pass->addSubpass(subpass);
}

}  // namespace my::rg
