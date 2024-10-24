#include "pass_creator.h"

#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "rendering/render_manager.h"
#include "rendering/rendering_dvars.h"

namespace my::rg {

/// Gbuffer
static void gbufferPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    auto& ctx = gm.getContext();
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    gm.setRenderTarget(p_draw_pass);

    gm.setViewport(Viewport(width, height));

    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    gm.clear(p_draw_pass, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT, clear_color);

    PassContext& pass = gm.main_pass;
    gm.bindUniformSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.bindUniformSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        gm.setPipelineState(has_bone ? PROGRAM_GBUFFER_ANIMATED : PROGRAM_GBUFFER_STATIC);

        if (draw.flags) {
            gm.setStencilRef(draw.flags);
        }

        gm.bindUniformSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.setMesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            const MaterialConstantBuffer& material = gm.m_context.material_cache.buffer[subset.material_idx];
            gm.bindTexture(Dimension::TEXTURE_2D, material.u_base_color_map_handle, u_base_color_map_slot);
            gm.bindTexture(Dimension::TEXTURE_2D, material.u_normal_map_handle, u_normal_map_slot);
            gm.bindTexture(Dimension::TEXTURE_2D, material.u_material_map_handle, u_material_map_slot);

            gm.bindUniformSlot<MaterialConstantBuffer>(ctx.material_uniform.get(), subset.material_idx);

            // @TODO: set material

            gm.drawElements(subset.index_count, subset.index_offset);

            // @TODO: unbind
        }

        if (draw.flags) {
            gm.setStencilRef(0);
        }
    }
}

void RenderPassCreator::addGBufferPass() {
    GraphicsManager& manager = GraphicsManager::singleton();

    int p_width = m_config.frame_width;
    int p_height = m_config.frame_height;

    // @TODO: decouple sampler and render target
    auto gbuffer_depth = manager.createRenderTarget(RenderTargetDesc(RESOURCE_GBUFFER_DEPTH,
                                                                     PixelFormat::D24_UNORM_S8_UINT,
                                                                     AttachmentType::DEPTH_STENCIL_2D,
                                                                     p_width, p_height),
                                                    nearest_sampler());

    auto attachment0 = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_BASE_COLOR,
                                                                    PixelFormat::R11G11B10_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    auto attachment1 = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_POSITION,
                                                                    PixelFormat::R16G16B16_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    auto attachment2 = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_NORMAL,
                                                                    PixelFormat::R16G16B16_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    auto attachment3 = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_MATERIAL,
                                                                    PixelFormat::R11G11B10_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  nearest_sampler());

    RenderPassDesc desc;
    desc.name = RenderPassName::GBUFFER;
    auto pass = m_graph.createPass(desc);
    auto draw_pass = manager.createDrawPass(DrawPassDesc{
        .color_attachments = { attachment0, attachment1, attachment2, attachment3 },
        .depth_attachment = gbuffer_depth,
        .exec_func = gbufferPassFunc,
    });
    pass->addDrawPass(draw_pass);
}

/// Shadow
static void pointShadowPassFunc(const DrawPass* p_draw_pass, int p_pass_id) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    auto& ctx = gm.getContext();

    auto& pass_ptr = gm.point_shadow_passes[p_pass_id];
    if (!pass_ptr) {
        return;
    }

    // prepare render data
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    // @TODO: instead of render the same object 6 times
    // set up different object list for different pass
    const PassContext& pass = *pass_ptr.get();

    const auto& light_matrices = pass.light_component.getMatrices();
    for (int i = 0; i < 6; ++i) {
        g_point_shadow_cache.cache.g_point_light_matrix = light_matrices[i];
        g_point_shadow_cache.cache.g_point_light_position = pass.light_component.getPosition();
        g_point_shadow_cache.cache.g_point_light_far = pass.light_component.getMaxDistance();
        g_point_shadow_cache.update();

        gm.setRenderTarget(p_draw_pass, i);
        gm.clear(p_draw_pass, CLEAR_DEPTH_BIT);

        Viewport viewport{ width, height };
        gm.setViewport(viewport);

        for (const auto& draw : pass.draws) {
            bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                gm.bindUniformSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
            }

            gm.setPipelineState(has_bone ? PROGRAM_POINT_SHADOW_ANIMATED : PROGRAM_POINT_SHADOW_STATIC);

            gm.bindUniformSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

            gm.setMesh(draw.mesh_data);
            gm.drawElements(draw.mesh_data->index_count);
        }
    }
}

static void shadowPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::singleton();
    auto& ctx = gm.getContext();

    gm.setRenderTarget(p_draw_pass);
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    gm.clear(p_draw_pass, CLEAR_DEPTH_BIT);

    Viewport viewport{ width, height };
    gm.setViewport(viewport);

    PassContext& pass = gm.shadow_passes[0];
    gm.bindUniformSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.bindUniformSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        // @TODO: sort the objects so there's no need to switch pipeline
        gm.setPipelineState(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);

        gm.bindUniformSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.setMesh(draw.mesh_data);
        gm.drawElements(draw.mesh_data->index_count);
    }

    gm.unsetRenderTarget();
}

void RenderPassCreator::addShadowPass() {
    GraphicsManager& manager = GraphicsManager::singleton();

    const int shadow_res = DVAR_GET_INT(r_shadow_res);
    DEV_ASSERT(math::isPowerOfTwo(shadow_res));
    const int point_shadow_res = DVAR_GET_INT(r_point_shadow_res);
    DEV_ASSERT(math::isPowerOfTwo(point_shadow_res));

    auto shadow_map = manager.createRenderTarget(RenderTargetDesc{ RESOURCE_SHADOW_MAP,
                                                                   PixelFormat::D32_FLOAT,
                                                                   AttachmentType::SHADOW_2D,
                                                                   1 * shadow_res, shadow_res },
                                                 shadow_map_sampler());
    RenderPassDesc desc;
    desc.name = RenderPassName::SHADOW;
    auto pass = m_graph.createPass(desc);
    {
        auto draw_pass = manager.createDrawPass(DrawPassDesc{
            .depth_attachment = shadow_map,
            .exec_func = shadowPassFunc,
        });
        pass->addDrawPass(draw_pass);
    }

    // @TODO: refactor
    DrawPassExecuteFunc funcs[] = {
        [](const DrawPass* p_draw_pass) {
            pointShadowPassFunc(p_draw_pass, 0);
        },
        [](const DrawPass* p_draw_pass) {
            pointShadowPassFunc(p_draw_pass, 1);
        },
        [](const DrawPass* p_draw_pass) {
            pointShadowPassFunc(p_draw_pass, 2);
        },
        [](const DrawPass* p_draw_pass) {
            pointShadowPassFunc(p_draw_pass, 3);
        },
    };

    static_assert(array_length(funcs) == MAX_LIGHT_CAST_SHADOW_COUNT);

    if (manager.getBackend() == Backend::OPENGL) {
        for (int i = 0; i < MAX_LIGHT_CAST_SHADOW_COUNT; ++i) {
            auto point_shadow_map = manager.createRenderTarget(RenderTargetDesc{ static_cast<RenderTargetResourceName>(RESOURCE_POINT_SHADOW_MAP_0 + i),
                                                                                 PixelFormat::D32_FLOAT,
                                                                                 AttachmentType::SHADOW_CUBE_MAP,
                                                                                 point_shadow_res, point_shadow_res },
                                                               shadow_cube_map_sampler());

            auto draw_pass = manager.createDrawPass(DrawPassDesc{
                .depth_attachment = point_shadow_map,
                .exec_func = funcs[i],
            });
            pass->addDrawPass(draw_pass);
        }
    }
}

/// Lighting
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

    // @TODO: [SCRUM-28] refactor 
    gm.setRenderTarget(nullptr);
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

/// Tone
static void tonePassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::singleton();
    gm.setRenderTarget(p_draw_pass);

    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto depth_buffer = p_draw_pass->depth_attachment;
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    // draw billboards

    // HACK:
    if (DVAR_GET_BOOL(r_debug_vxgi) && gm.getBackend() == Backend::OPENGL) {
        // @TODO: remove
        extern void debug_vxgi_pass_func(const DrawPass* p_draw_pass);
        debug_vxgi_pass_func(p_draw_pass);
    } else {
        auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
            std::shared_ptr<RenderTarget> resource = gm.findRenderTarget(p_name);
            if (!resource) {
                return;
            }

            gm.bindTexture(p_dimension, resource->texture->get_handle(), p_slot);
        };
        bind_slot(RESOURCE_LIGHTING, g_texture_lighting_slot);

        gm.setViewport(Viewport(width, height));
        gm.clear(p_draw_pass, CLEAR_COLOR_BIT);

        gm.setPipelineState(PROGRAM_TONE);
        RenderManager::singleton().draw_quad();

        gm.unbindTexture(Dimension::TEXTURE_2D, g_texture_lighting_slot);
    }
}

void RenderPassCreator::addTonePass() {
    GraphicsManager& gm = GraphicsManager::singleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::TONE;
    desc.dependencies = { RenderPassName::BLOOM };

    auto pass = m_graph.createPass(desc);

    int width = m_config.frame_width;
    int height = m_config.frame_height;

    auto attachment = gm.createRenderTarget(RenderTargetDesc{ RESOURCE_TONE,
                                                              PixelFormat::R11G11B10_FLOAT,
                                                              AttachmentType::COLOR_2D,
                                                              width, height },
                                            nearest_sampler());

    auto gbuffer_depth = gm.findRenderTarget(RESOURCE_GBUFFER_DEPTH);

    auto draw_pass = gm.createDrawPass(DrawPassDesc{
        .color_attachments = { attachment },
        .depth_attachment = gbuffer_depth,
        .exec_func = tonePassFunc,
    });
    pass->addDrawPass(draw_pass);
}

}  // namespace my::rg
