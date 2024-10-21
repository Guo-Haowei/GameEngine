#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_graph/pass_creator.h"
#include "rendering/render_manager.h"

// @TODO: refactor
#include "core/framework/scene_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"

namespace my::rg {

static void lightingPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    gm.setRenderTarget(p_draw_pass);

    Viewport viewport(width, height);
    gm.setViewport(viewport);
    gm.clear(p_draw_pass, CLEAR_COLOR_BIT);
    gm.setPipelineState(PROGRAM_LIGHTING);

    // @TODO: refactor pass to auto bind resources,
    // and make it a class so don't do a map search every frame
    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<RenderTarget> resource = gm.findRenderTarget(p_name);
        if (!resource) {
            return;
        }

        gm.bindTexture(p_dimension, resource->texture->get_handle(), p_slot);
    };

    // bind common textures
    bind_slot(RESOURCE_GBUFFER_BASE_COLOR, u_gbuffer_base_color_map_slot);
    bind_slot(RESOURCE_GBUFFER_POSITION, u_gbuffer_position_map_slot);
    bind_slot(RESOURCE_GBUFFER_NORMAL, u_gbuffer_normal_map_slot);
    bind_slot(RESOURCE_GBUFFER_MATERIAL, u_gbuffer_material_map_slot);
    bind_slot(RESOURCE_SHADOW_MAP, u_shadow_map_slot);

    // @TODO: fix it
    RenderManager::singleton().draw_quad();

    PassContext& pass = gm.main_pass;
    gm.bindUniformSlot<PerPassConstantBuffer>(gm.m_context.pass_uniform.get(), pass.pass_idx);

    // if (0) {
    //     // draw billboard grass here for now
    //     manager.setPipelineState(PROGRAM_BILLBOARD);
    //     manager.setMesh(&g_grass);
    //     glDrawElementsInstanced(GL_TRIANGLES, g_grass.index_count, GL_UNSIGNED_INT, 0, 64);
    // }

    // @TODO: fix skybox
    if (gm.getBackend() == Backend::OPENGL) {
        GraphicsManager::singleton().setPipelineState(PROGRAM_ENV_SKYBOX);
        RenderManager::singleton().draw_skybox();
    }

    // unbind stuff
    gm.unbindTexture(Dimension::TEXTURE_2D, u_gbuffer_base_color_map_slot);
    gm.unbindTexture(Dimension::TEXTURE_2D, u_gbuffer_position_map_slot);
    gm.unbindTexture(Dimension::TEXTURE_2D, u_gbuffer_normal_map_slot);
    gm.unbindTexture(Dimension::TEXTURE_2D, u_gbuffer_material_map_slot);
    gm.unbindTexture(Dimension::TEXTURE_2D, u_shadow_map_slot);
}

void RenderPassCreator::addLightingPass() {
    GraphicsManager& manager = GraphicsManager::singleton();

    auto gbuffer_depth = manager.findRenderTarget(RESOURCE_GBUFFER_DEPTH);

    auto lighting_attachment = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_LIGHTING,
                                                                            PixelFormat::R11G11B10_FLOAT,
                                                                            AttachmentType::COLOR_2D,
                                                                            m_config.frame_width, m_config.frame_height },
                                                          nearest_sampler());

    RenderPassDesc desc;
    desc.name = RenderPassName::LIGHTING;

    desc.dependencies = { RenderPassName::GBUFFER };
    if (m_config.enable_shadow) {
        desc.dependencies.push_back(RenderPassName::SHADOW);
    }
    if (m_config.enable_voxel_gi) {
        desc.dependencies.push_back(RenderPassName::VOXELIZATION);
    }
    if (m_config.enable_ibl) {
        desc.dependencies.push_back(RenderPassName::ENV);
    }

    auto pass = m_graph.createPass(desc);
    auto drawpass = manager.createDrawPass(DrawPassDesc{
        .color_attachments = { lighting_attachment },
        .depth_attachment = gbuffer_depth,
        .exec_func = lightingPassFunc,
    });
    pass->addDrawPass(drawpass);
}

}  // namespace my::rg
