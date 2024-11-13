#include "pass_creator.h"

#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/math/geomath.h"
#include "particle_defines.h"
#include "rendering/render_manager.h"
#include "rendering/rendering_dvars.h"

// @TODO: this is temporary
#include "core/framework/scene_manager.h"

namespace my::rg {

/// Gbuffer
static void GbufferPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    gm.SetRenderTarget(p_draw_pass);

    gm.SetViewport(Viewport(width, height));

    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT, clear_color);

    PassContext& pass = gm.m_mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.BindConstantBufferSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        gm.SetPipelineState(has_bone ? PROGRAM_GBUFFER_ANIMATED : PROGRAM_GBUFFER_STATIC);

        if (draw.flags) {
            gm.SetStencilRef(draw.flags);
        }

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.SetMesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            const MaterialConstantBuffer& material = gm.m_context.material_cache.buffer[subset.material_idx];
            gm.BindTexture(Dimension::TEXTURE_2D, material.t_baseColorMap_handle, t_baseColorMapSlot);
            gm.BindTexture(Dimension::TEXTURE_2D, material.t_normalMap_handle, t_normalMapSlot);
            gm.BindTexture(Dimension::TEXTURE_2D, material.t_materialMap_handle, t_materialMapSlot);

            gm.BindConstantBufferSlot<MaterialConstantBuffer>(ctx.material_uniform.get(), subset.material_idx);

            // @TODO: set material

            gm.DrawElements(subset.index_count, subset.index_offset);

            // @TODO: unbind
        }

        if (draw.flags) {
            gm.SetStencilRef(0);
        }
    }
}

void RenderPassCreator::AddGBufferPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    int p_width = m_config.frame_width;
    int p_height = m_config.frame_height;

    // @TODO: decouple sampler and render target
    auto gbuffer_depth = manager.CreateRenderTarget(RenderTargetDesc(RESOURCE_GBUFFER_DEPTH,
                                                                     PixelFormat::R24G8_TYPELESS,
                                                                     // PixelFormat::D24_UNORM_S8_UINT,
                                                                     AttachmentType::DEPTH_STENCIL_2D,
                                                                     p_width, p_height),
                                                    PointClampSampler());

    auto attachment0 = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_BASE_COLOR,
                                                                    PixelFormat::R11G11B10_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  PointClampSampler());

    auto attachment1 = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_POSITION,
                                                                    PixelFormat::R16G16B16_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  PointClampSampler());

    auto attachment2 = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_NORMAL,
                                                                    PixelFormat::R16G16B16_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  PointClampSampler());

    auto attachment3 = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_GBUFFER_MATERIAL,
                                                                    PixelFormat::R11G11B10_FLOAT,
                                                                    AttachmentType::COLOR_2D,
                                                                    p_width, p_height },
                                                  PointClampSampler());

    RenderPassDesc desc;
    desc.name = RenderPassName::GBUFFER;
    auto pass = m_graph.CreatePass(desc);
    auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
        .color_attachments = { attachment0, attachment1, attachment2, attachment3 },
        .depth_attachment = gbuffer_depth,
        .exec_func = GbufferPassFunc,
    });
    pass->AddDrawPass(draw_pass);
}

/// Shadow
static void PointShadowPassFunc(const DrawPass* p_draw_pass, int p_pass_id) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();

    auto& pass_ptr = gm.m_pointShadowPasses[p_pass_id];
    if (!pass_ptr) {
        return;
    }

    // prepare render data
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    // @TODO: instead of render the same object 6 times
    // set up different object list for different pass
    const PassContext& pass = *pass_ptr.get();

    const auto& light_matrices = pass.light_component.GetMatrices();
    for (int i = 0; i < 6; ++i) {
        g_point_shadow_cache.cache.c_pointLightMatrix = light_matrices[i];
        g_point_shadow_cache.cache.c_pointLightPosition = pass.light_component.GetPosition();
        g_point_shadow_cache.cache.c_pointLightFar = pass.light_component.GetMaxDistance();
        g_point_shadow_cache.update();

        gm.SetRenderTarget(p_draw_pass, i);
        gm.Clear(p_draw_pass, CLEAR_DEPTH_BIT, nullptr, i);

        gm.SetViewport(Viewport(width, height));

        for (const auto& draw : pass.draws) {
            bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                gm.BindConstantBufferSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
            }

            gm.SetPipelineState(has_bone ? PROGRAM_POINT_SHADOW_ANIMATED : PROGRAM_POINT_SHADOW_STATIC);

            gm.BindConstantBufferSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

            gm.SetMesh(draw.mesh_data);
            gm.DrawElements(draw.mesh_data->indexCount);
        }
    }
}

static void ShadowPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();

    gm.SetRenderTarget(p_draw_pass);
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    gm.Clear(p_draw_pass, CLEAR_DEPTH_BIT);

    gm.SetViewport(Viewport(width, height));

    PassContext& pass = gm.m_shadowPasses[0];
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    for (const auto& draw : pass.draws) {
        bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.BindConstantBufferSlot<BoneConstantBuffer>(ctx.bone_uniform.get(), draw.bone_idx);
        }

        // @TODO: sort the objects so there's no need to switch pipeline
        gm.SetPipelineState(has_bone ? PROGRAM_DPETH_ANIMATED : PROGRAM_DPETH_STATIC);

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(ctx.batch_uniform.get(), draw.batch_idx);

        gm.SetMesh(draw.mesh_data);
        gm.DrawElements(draw.mesh_data->indexCount);
    }

    gm.UnsetRenderTarget();
}

void RenderPassCreator::AddShadowPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    const int shadow_res = DVAR_GET_INT(r_shadow_res);
    DEV_ASSERT(math::IsPowerOfTwo(shadow_res));
    const int point_shadow_res = DVAR_GET_INT(r_point_shadow_res);
    DEV_ASSERT(math::IsPowerOfTwo(point_shadow_res));

    auto shadow_map = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_SHADOW_MAP,
                                                                   PixelFormat::D32_FLOAT,
                                                                   AttachmentType::SHADOW_2D,
                                                                   1 * shadow_res, shadow_res },
                                                 shadow_map_sampler());
    RenderPassDesc desc;
    desc.name = RenderPassName::SHADOW;
    auto pass = m_graph.CreatePass(desc);
    {
        auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
            .depth_attachment = shadow_map,
            .exec_func = ShadowPassFunc,
        });
        pass->AddDrawPass(draw_pass);
    }

    // @TODO: refactor
    DrawPassExecuteFunc funcs[] = {
        [](const DrawPass* p_draw_pass) {
            PointShadowPassFunc(p_draw_pass, 0);
        },
        [](const DrawPass* p_draw_pass) {
            PointShadowPassFunc(p_draw_pass, 1);
        },
        [](const DrawPass* p_draw_pass) {
            PointShadowPassFunc(p_draw_pass, 2);
        },
        [](const DrawPass* p_draw_pass) {
            PointShadowPassFunc(p_draw_pass, 3);
        },
    };

    static_assert(array_length(funcs) == MAX_LIGHT_CAST_SHADOW_COUNT);

    {
        for (int i = 0; i < MAX_LIGHT_CAST_SHADOW_COUNT; ++i) {
            auto point_shadowMap = manager.CreateRenderTarget(RenderTargetDesc{ static_cast<RenderTargetResourceName>(RESOURCE_POINT_SHADOW_MAP_0 + i),
                                                                                PixelFormat::D32_FLOAT,
                                                                                AttachmentType::SHADOW_CUBE_MAP,
                                                                                point_shadow_res, point_shadow_res },
                                                              shadow_cube_map_sampler());

            auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
                .depth_attachment = point_shadowMap,
                .exec_func = funcs[i],
            });
            pass->AddDrawPass(draw_pass);
        }
    }
}

/// Lighting
static void LightingPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    gm.SetRenderTarget(p_draw_pass);

    gm.SetViewport(Viewport(width, height));
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);
    gm.SetPipelineState(PROGRAM_LIGHTING);

    // @TODO: refactor pass to auto bind resources,
    // and make it a class so don't do a map search every frame
    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<RenderTarget> resource = gm.FindRenderTarget(p_name);
        if (!resource) {
            return;
        }

        gm.BindTexture(p_dimension, resource->texture->GetHandle(), p_slot);
    };

    // bind common textures
    bind_slot(RESOURCE_GBUFFER_BASE_COLOR, t_gbufferBaseColorMapSlot);
    bind_slot(RESOURCE_GBUFFER_POSITION, t_gbufferPositionMapSlot);
    bind_slot(RESOURCE_GBUFFER_NORMAL, t_gbufferNormalMapSlot);
    bind_slot(RESOURCE_GBUFFER_MATERIAL, t_gbufferMaterialMapSlot);
    bind_slot(RESOURCE_GBUFFER_DEPTH, t_gbufferDepthSlot);

    bind_slot(RESOURCE_SHADOW_MAP, t_shadowMapSlot);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_0, t_pointShadow0Slot, Dimension::TEXTURE_CUBE);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_1, t_pointShadow1Slot, Dimension::TEXTURE_CUBE);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_2, t_pointShadow2Slot, Dimension::TEXTURE_CUBE);
    bind_slot(RESOURCE_POINT_SHADOW_MAP_3, t_pointShadow3Slot, Dimension::TEXTURE_CUBE);

    // @TODO: fix it
    RenderManager::GetSingleton().draw_quad();

    PassContext& pass = gm.m_mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.m_context.pass_uniform.get(), pass.pass_idx);

    // if (0) {
    //     // draw billboard grass here for now
    //     manager.SetPipelineState(PROGRAM_BILLBOARD);
    //     manager.setMesh(&g_grass);
    //     glDrawElementsInstanced(GL_TRIANGLES, g_grass.index_count, GL_UNSIGNED_INT, 0, 64);
    // }

    // @TODO: fix skybox
    // if (gm.GetBackend() == Backend::OPENGL) {
    //     GraphicsManager::GetSingleton().SetPipelineState(PROGRAM_ENV_SKYBOX);
    //     RenderManager::GetSingleton().draw_skybox();
    // }

    // unbind stuff
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferBaseColorMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferPositionMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferNormalMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferMaterialMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferDepthSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_shadowMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow0Slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow1Slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow2Slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow3Slot);

    // @TODO: [SCRUM-28] refactor
    gm.UnsetRenderTarget();
}

void RenderPassCreator::AddLightingPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    auto gbuffer_depth = manager.FindRenderTarget(RESOURCE_GBUFFER_DEPTH);

    auto lighting_attachment = manager.CreateRenderTarget(RenderTargetDesc{ RESOURCE_LIGHTING,
                                                                            PixelFormat::R11G11B10_FLOAT,
                                                                            AttachmentType::COLOR_2D,
                                                                            m_config.frame_width, m_config.frame_height },
                                                          PointClampSampler());

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

    auto pass = m_graph.CreatePass(desc);
    auto drawpass = manager.CreateDrawPass(DrawPassDesc{
        .color_attachments = { lighting_attachment },
        .exec_func = LightingPassFunc,
    });
    pass->AddDrawPass(drawpass);
}

/// Emitter
static void EmitterPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();
    auto [width, height] = p_draw_pass->depth_attachment->getSize();

    gm.SetRenderTarget(p_draw_pass);
    gm.SetViewport(Viewport(width, height));

    PassContext& pass = gm.m_mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(ctx.pass_uniform.get(), pass.pass_idx);

    const Scene& scene = SceneManager::GetScene();
    int particle_idx = 0;
    for (auto [id, emitter] : scene.m_ParticleEmitterComponents) {

        gm.BindConstantBufferSlot<EmitterConstantBuffer>(ctx.emitter_uniform.get(), particle_idx++);

        gm.BindStructuredBuffer(GetGlobalParticleCounterSlot(), emitter.counterBuffer.get());
        gm.BindStructuredBuffer(GetGlobalDeadIndicesSlot(), emitter.deadBuffer.get());
        gm.BindStructuredBuffer(GetGlobalAliveIndicesPreSimSlot(), emitter.aliveBuffer[emitter.GetPreIndex()].get());
        gm.BindStructuredBuffer(GetGlobalAliveIndicesPostSimSlot(), emitter.aliveBuffer[emitter.GetPostIndex()].get());
        gm.BindStructuredBuffer(GetGlobalParticleDataSlot(), emitter.particleBuffer.get());

        gm.SetPipelineState(PROGRAM_PARTICLE_KICKOFF);
        gm.Dispatch(1, 1, 1);

        gm.SetPipelineState(PROGRAM_PARTICLE_EMIT);
        gm.Dispatch(MAX_PARTICLE_COUNT / PARTICLE_LOCAL_SIZE, 1, 1);

        gm.SetPipelineState(PROGRAM_PARTICLE_SIM);
        gm.Dispatch(MAX_PARTICLE_COUNT / PARTICLE_LOCAL_SIZE, 1, 1);

        gm.UnbindStructuredBuffer(GetGlobalParticleCounterSlot());
        gm.UnbindStructuredBuffer(GetGlobalDeadIndicesSlot());
        gm.UnbindStructuredBuffer(GetGlobalAliveIndicesPreSimSlot());
        gm.UnbindStructuredBuffer(GetGlobalAliveIndicesPostSimSlot());
        gm.UnbindStructuredBuffer(GetGlobalParticleDataSlot());

        // Renderering
        gm.SetPipelineState(PROGRAM_PARTICLE_RENDERING);

        gm.BindStructuredBufferSRV(GetGlobalParticleDataSlot(), emitter.particleBuffer.get());
        RenderManager::GetSingleton().draw_quad_instanced(MAX_PARTICLE_COUNT);
        gm.UnbindStructuredBufferSRV(GetGlobalParticleDataSlot());
    }

    gm.UnsetRenderTarget();
}

void RenderPassCreator::AddEmitterPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::EMITTER;

    desc.dependencies = { RenderPassName::LIGHTING };

    auto pass = m_graph.CreatePass(desc);
    auto drawpass = manager.CreateDrawPass(DrawPassDesc{
        .color_attachments = { manager.FindRenderTarget(RESOURCE_LIGHTING) },
        .depth_attachment = manager.FindRenderTarget(RESOURCE_GBUFFER_DEPTH),
        .exec_func = EmitterPassFunc,
    });
    pass->AddDrawPass(drawpass);
}

/// Bloom
static void BloomFunc(const DrawPass*) {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    // Step 1, select pixels contribute to bloom
    {
        gm.SetPipelineState(PROGRAM_BLOOM_SETUP);
        auto input = gm.FindRenderTarget(RESOURCE_LIGHTING);
        auto output = gm.FindRenderTarget(RESOURCE_BLOOM_0);

        auto [width, height] = input->getSize();
        const uint32_t work_group_x = math::CeilingDivision(width, 16);
        const uint32_t work_group_y = math::CeilingDivision(height, 16);

        gm.BindTexture(Dimension::TEXTURE_2D, input->texture->GetHandle(), t_bloomInputImageSlot);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture.get());
        gm.Dispatch(work_group_x, work_group_y, 1);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, nullptr);
        gm.UnbindTexture(Dimension::TEXTURE_2D, t_bloomInputImageSlot);
    }

    // Step 2, down sampling
    gm.SetPipelineState(PROGRAM_BLOOM_DOWNSAMPLE);
    for (int i = 1; i < BLOOM_MIP_CHAIN_MAX; ++i) {
        auto input = gm.FindRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));
        auto output = gm.FindRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));

        DEV_ASSERT(input && output);

        auto [width, height] = output->getSize();
        const uint32_t work_group_x = math::CeilingDivision(width, 16);
        const uint32_t work_group_y = math::CeilingDivision(height, 16);

        gm.BindTexture(Dimension::TEXTURE_2D, input->texture->GetHandle(), t_bloomInputImageSlot);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture.get());
        gm.Dispatch(work_group_x, work_group_y, 1);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, nullptr);
        gm.UnbindTexture(Dimension::TEXTURE_2D, t_bloomInputImageSlot);
    }

    // Step 3, up sampling
    gm.SetPipelineState(PROGRAM_BLOOM_UPSAMPLE);
    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto input = gm.FindRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));
        auto output = gm.FindRenderTarget(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));

        auto [width, height] = output->getSize();
        const uint32_t work_group_x = math::CeilingDivision(width, 16);
        const uint32_t work_group_y = math::CeilingDivision(height, 16);

        gm.BindTexture(Dimension::TEXTURE_2D, input->texture->GetHandle(), t_bloomInputImageSlot);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output->texture.get());
        gm.Dispatch(work_group_x, work_group_y, 1);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, nullptr);
        gm.UnbindTexture(Dimension::TEXTURE_2D, t_bloomInputImageSlot);
    }
}

void RenderPassCreator::AddBloomPass() {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::BLOOM;
    desc.dependencies = { RenderPassName::LIGHTING };
    auto pass = m_graph.CreatePass(desc);

    int width = m_config.frame_width;
    int height = m_config.frame_height;
    for (int i = 0; i < BLOOM_MIP_CHAIN_MAX; ++i, width /= 2, height /= 2) {
        DEV_ASSERT(width > 1);
        DEV_ASSERT(height > 1);

        LOG_WARN("bloom size {}x{}", width, height);

        auto attachment = gm.CreateRenderTarget(RenderTargetDesc(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i),
                                                                 PixelFormat::R11G11B10_FLOAT,
                                                                 AttachmentType::COLOR_2D,
                                                                 width, height, false, true),
                                                LinearClampSampler());
    }

    auto draw_pass = gm.CreateDrawPass(DrawPassDesc{
        .color_attachments = {},
        .exec_func = BloomFunc,
    });
    pass->AddDrawPass(draw_pass);
}

/// Tone
/// Change to post processing?
static void TonePassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);

    DEV_ASSERT(!p_draw_pass->color_attachments.empty());
    auto depth_buffer = p_draw_pass->depth_attachment;
    auto [width, height] = p_draw_pass->color_attachments[0]->getSize();

    // draw billboards

    // HACK:
    if (DVAR_GET_BOOL(r_debug_vxgi) && gm.GetBackend() == Backend::OPENGL) {
        // @TODO: remove
        extern void debug_vxgi_pass_func(const DrawPass* p_draw_pass);
        debug_vxgi_pass_func(p_draw_pass);
    } else {
        auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
            std::shared_ptr<RenderTarget> resource = gm.FindRenderTarget(p_name);
            if (!resource) {
                return;
            }

            gm.BindTexture(p_dimension, resource->texture->GetHandle(), p_slot);
        };
        bind_slot(RESOURCE_LIGHTING, t_textureLightingSlot);
        bind_slot(RESOURCE_BLOOM_0, t_bloomInputImageSlot);

        gm.SetViewport(Viewport(width, height));
        gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);

        gm.SetPipelineState(PROGRAM_TONE);
        RenderManager::GetSingleton().draw_quad();

        gm.UnbindTexture(Dimension::TEXTURE_2D, t_textureLightingSlot);
        gm.UnbindTexture(Dimension::TEXTURE_2D, t_bloomInputImageSlot);
    }
}

void RenderPassCreator::AddTonePass() {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::TONE;
    desc.dependencies = { RenderPassName::BLOOM };

    auto pass = m_graph.CreatePass(desc);

    int width = m_config.frame_width;
    int height = m_config.frame_height;

    auto attachment = gm.CreateRenderTarget(RenderTargetDesc{ RESOURCE_TONE,
                                                              PixelFormat::R11G11B10_FLOAT,
                                                              AttachmentType::COLOR_2D,
                                                              width, height },
                                            PointClampSampler());

    auto gbuffer_depth = gm.FindRenderTarget(RESOURCE_GBUFFER_DEPTH);

    auto draw_pass = gm.CreateDrawPass(DrawPassDesc{
        .color_attachments = { attachment },
        .depth_attachment = gbuffer_depth,
        .exec_func = TonePassFunc,
    });
    pass->AddDrawPass(draw_pass);
}

}  // namespace my::rg
