#include "pass_creator.h"

#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/math/geomath.h"
#include "particle_defines.h"
#include "rendering/graphics_dvars.h"
#include "rendering/render_manager.h"

// @TODO: this is temporary
#include "core/framework/scene_manager.h"

namespace my::rg {

/// Gbuffer
static void GbufferPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();
    const uint32_t width = p_draw_pass->depthAttachment->desc.width;
    const uint32_t height = p_draw_pass->depthAttachment->desc.height;

    gm.SetRenderTarget(p_draw_pass);

    gm.SetViewport(Viewport(width, height));

    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    if (gm.GetBackend() == Backend::D3D12) {
        clear_color[0] = 0.3f;
        clear_color[1] = 0.3f;
        clear_color[2] = 0.4f;
    }
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

    int p_width = m_config.frameWidth;
    int p_height = m_config.frameHeight;

    auto gbuffer_depth = manager.CreateGpuTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_DEPTH,
                                                                          PixelFormat::R24G8_TYPELESS,
                                                                          AttachmentType::DEPTH_STENCIL_2D,
                                                                          p_width, p_height),
                                                  PointClampSampler());

    auto attachment0 = manager.CreateGpuTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_BASE_COLOR,
                                                                        PixelFormat::R11G11B10_FLOAT,
                                                                        AttachmentType::COLOR_2D,
                                                                        p_width, p_height),
                                                PointClampSampler());

    auto attachment1 = manager.CreateGpuTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_POSITION,
                                                                        PixelFormat::R16G16B16_FLOAT,
                                                                        AttachmentType::COLOR_2D,
                                                                        p_width, p_height),
                                                PointClampSampler());

    auto attachment2 = manager.CreateGpuTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_NORMAL,
                                                                        PixelFormat::R16G16B16_FLOAT,
                                                                        AttachmentType::COLOR_2D,
                                                                        p_width, p_height),
                                                PointClampSampler());

    auto attachment3 = manager.CreateGpuTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_MATERIAL,
                                                                        PixelFormat::R11G11B10_FLOAT,
                                                                        AttachmentType::COLOR_2D,
                                                                        p_width, p_height),
                                                PointClampSampler());

    RenderPassDesc desc;
    desc.name = RenderPassName::GBUFFER;
    auto pass = m_graph.CreatePass(desc);
    auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
        .colorAttachments = { attachment0, attachment1, attachment2, attachment3 },
        .depthAttachment = gbuffer_depth,
        .execFunc = GbufferPassFunc,
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
    const uint32_t width = p_draw_pass->depthAttachment->desc.width;
    const uint32_t height = p_draw_pass->depthAttachment->desc.height;

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
    const uint32_t width = p_draw_pass->depthAttachment->desc.width;
    const uint32_t height = p_draw_pass->depthAttachment->desc.height;

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
}

void RenderPassCreator::AddShadowPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    const int shadow_res = DVAR_GET_INT(gfx_shadow_res);
    DEV_ASSERT(math::IsPowerOfTwo(shadow_res));
    const int point_shadow_res = DVAR_GET_INT(gfx_point_shadow_res);
    DEV_ASSERT(math::IsPowerOfTwo(point_shadow_res));

    auto shadow_map = manager.CreateGpuTexture(BuildDefaultTextureDesc(RESOURCE_SHADOW_MAP,
                                                                       PixelFormat::D32_FLOAT,
                                                                       AttachmentType::SHADOW_2D,
                                                                       1 * shadow_res, shadow_res),
                                               shadow_map_sampler());
    RenderPassDesc desc;
    desc.name = RenderPassName::SHADOW;
    auto pass = m_graph.CreatePass(desc);
    {
        auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
            .depthAttachment = shadow_map,
            .execFunc = ShadowPassFunc,
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
            auto point_shadowMap = manager.CreateGpuTexture(BuildDefaultTextureDesc(static_cast<RenderTargetResourceName>(RESOURCE_POINT_SHADOW_MAP_0 + i),
                                                                                    PixelFormat::D32_FLOAT,
                                                                                    AttachmentType::SHADOW_CUBE_MAP,
                                                                                    point_shadow_res, point_shadow_res),
                                                            shadow_cube_map_sampler());

            auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
                .depthAttachment = point_shadowMap,
                .execFunc = funcs[i],
            });
            pass->AddDrawPass(draw_pass);
        }
    }
}

/// Lighting
static void LightingPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    DEV_ASSERT(!p_draw_pass->colorAttachments.empty());
    const uint32_t width = p_draw_pass->colorAttachments[0]->desc.width;
    const uint32_t height = p_draw_pass->colorAttachments[0]->desc.height;

    gm.SetRenderTarget(p_draw_pass);

    gm.SetViewport(Viewport(width, height));
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);
    gm.SetPipelineState(PROGRAM_LIGHTING);

    // @TODO: refactor pass to auto bind resources,
    // and make it a class so don't do a map search every frame
    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<GpuTexture> resource = gm.FindGpuTexture(p_name);
        if (!resource) {
            return;
        }

        gm.BindTexture(p_dimension, resource->GetHandle(), p_slot);
    };

    // bind common textures
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
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_gbufferDepthSlot);
    gm.UnbindTexture(Dimension::TEXTURE_2D, t_shadowMapSlot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow0Slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow1Slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow2Slot);
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, t_pointShadow3Slot);
}

void RenderPassCreator::AddLightingPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    auto gbuffer_depth = manager.FindGpuTexture(RESOURCE_GBUFFER_DEPTH);

    auto lighting_attachment = manager.CreateGpuTexture(BuildDefaultTextureDesc(RESOURCE_LIGHTING,
                                                                                PixelFormat::R11G11B10_FLOAT,
                                                                                AttachmentType::COLOR_2D,
                                                                                m_config.frameWidth, m_config.frameHeight),
                                                        PointClampSampler());

    RenderPassDesc desc;
    desc.name = RenderPassName::LIGHTING;

    desc.dependencies = { RenderPassName::GBUFFER };
    if (m_config.enableShadow) {
        desc.dependencies.push_back(RenderPassName::SHADOW);
    }
    if (m_config.enableVxgi) {
        desc.dependencies.push_back(RenderPassName::VOXELIZATION);
    }
    if (m_config.enableIbl) {
        desc.dependencies.push_back(RenderPassName::ENV);
    }

    auto pass = m_graph.CreatePass(desc);
    auto drawpass = manager.CreateDrawPass(DrawPassDesc{
        .colorAttachments = { lighting_attachment },
        .execFunc = LightingPassFunc,
    });
    pass->AddDrawPass(drawpass);
}

/// Emitter
static void EmitterPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& ctx = gm.GetContext();
    const uint32_t width = p_draw_pass->depthAttachment->desc.width;
    const uint32_t height = p_draw_pass->depthAttachment->desc.height;

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
}

void RenderPassCreator::AddEmitterPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::EMITTER;

    desc.dependencies = { RenderPassName::LIGHTING };

    auto pass = m_graph.CreatePass(desc);
    auto drawpass = manager.CreateDrawPass(DrawPassDesc{
        .colorAttachments = { manager.FindGpuTexture(RESOURCE_LIGHTING) },
        .depthAttachment = manager.FindGpuTexture(RESOURCE_GBUFFER_DEPTH),
        .execFunc = EmitterPassFunc,
    });
    pass->AddDrawPass(drawpass);
}

/// Bloom
static void BloomFunc(const DrawPass*) {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    // Step 1, select pixels contribute to bloom
    {
        gm.SetPipelineState(PROGRAM_BLOOM_SETUP);
        auto input = gm.FindGpuTexture(RESOURCE_LIGHTING);
        auto output = gm.FindGpuTexture(RESOURCE_BLOOM_0);

        const uint32_t width = input->desc.width;
        const uint32_t height = input->desc.height;
        const uint32_t work_group_x = math::CeilingDivision(width, 16);
        const uint32_t work_group_y = math::CeilingDivision(height, 16);

        gm.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), t_bloomInputImageSlot);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output.get());
        gm.Dispatch(work_group_x, work_group_y, 1);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, nullptr);
        gm.UnbindTexture(Dimension::TEXTURE_2D, t_bloomInputImageSlot);
    }

    // Step 2, down sampling
    gm.SetPipelineState(PROGRAM_BLOOM_DOWNSAMPLE);
    for (int i = 1; i < BLOOM_MIP_CHAIN_MAX; ++i) {
        auto input = gm.FindGpuTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));
        auto output = gm.FindGpuTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));

        DEV_ASSERT(input && output);

        const uint32_t width = input->desc.width;
        const uint32_t height = input->desc.height;
        const uint32_t work_group_x = math::CeilingDivision(width, 16);
        const uint32_t work_group_y = math::CeilingDivision(height, 16);

        gm.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), t_bloomInputImageSlot);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output.get());
        gm.Dispatch(work_group_x, work_group_y, 1);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, nullptr);
        gm.UnbindTexture(Dimension::TEXTURE_2D, t_bloomInputImageSlot);
    }

    // Step 3, up sampling
    gm.SetPipelineState(PROGRAM_BLOOM_UPSAMPLE);
    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto input = gm.FindGpuTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i));
        auto output = gm.FindGpuTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));

        const uint32_t width = output->desc.width;
        const uint32_t height = output->desc.height;
        const uint32_t work_group_x = math::CeilingDivision(width, 16);
        const uint32_t work_group_y = math::CeilingDivision(height, 16);

        gm.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), t_bloomInputImageSlot);
        gm.SetUnorderedAccessView(IMAGE_BLOOM_DOWNSAMPLE_OUTPUT_SLOT, output.get());
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

    int width = m_config.frameWidth;
    int height = m_config.frameHeight;
    for (int i = 0; i < BLOOM_MIP_CHAIN_MAX; ++i, width /= 2, height /= 2) {
        DEV_ASSERT(width > 1);
        DEV_ASSERT(height > 1);

        LOG_WARN("bloom size {}x{}", width, height);

        GpuTextureDesc texture_desc = BuildDefaultTextureDesc(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i),
                                                              PixelFormat::R11G11B10_FLOAT,
                                                              AttachmentType::COLOR_2D,
                                                              width, height);
        texture_desc.bindFlags |= BIND_UNORDERED_ACCESS;
        auto attachment = gm.CreateGpuTexture(texture_desc, LinearClampSampler());
    }

    auto draw_pass = gm.CreateDrawPass(DrawPassDesc{
        .colorAttachments = {},
        .execFunc = BloomFunc,
    });
    pass->AddDrawPass(draw_pass);
}

/// Tone
/// Change to post processing?
static void TonePassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);

    DEV_ASSERT(!p_draw_pass->colorAttachments.empty());
    auto depth_buffer = p_draw_pass->depthAttachment;
    const uint32_t width = p_draw_pass->colorAttachments[0]->desc.width;
    const uint32_t height = p_draw_pass->colorAttachments[0]->desc.height;

    // draw billboards

    // HACK:
    if (DVAR_GET_BOOL(gfx_debug_vxgi) && gm.GetBackend() == Backend::OPENGL) {
        // @TODO: remove
        extern void debug_vxgi_pass_func(const DrawPass* p_draw_pass);
        debug_vxgi_pass_func(p_draw_pass);
    } else {
        auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
            std::shared_ptr<GpuTexture> resource = gm.FindGpuTexture(p_name);
            if (!resource) {
                return;
            }

            gm.BindTexture(p_dimension, resource->GetHandle(), p_slot);
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

    int width = m_config.frameWidth;
    int height = m_config.frameHeight;

    auto attachment = gm.CreateGpuTexture(BuildDefaultTextureDesc(RESOURCE_TONE,
                                                                  PixelFormat::R11G11B10_FLOAT,
                                                                  AttachmentType::COLOR_2D,
                                                                  width, height),
                                          PointClampSampler());

    auto gbuffer_depth = gm.FindGpuTexture(RESOURCE_GBUFFER_DEPTH);

    auto draw_pass = gm.CreateDrawPass(DrawPassDesc{
        .colorAttachments = { attachment },
        .depthAttachment = gbuffer_depth,
        .execFunc = TonePassFunc,
    });
    pass->AddDrawPass(draw_pass);
}

/// Create pre-defined passes
void RenderPassCreator::CreateDummy(RenderGraph& p_graph) {
    const ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    const int w = frame_size.x;
    const int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frameWidth = w;
    config.frameHeight = h;
    config.enableBloom = false;
    config.enableIbl = false;
    config.enableVxgi = false;
    RenderPassCreator creator(config, p_graph);

    creator.AddGBufferPass();

    p_graph.Compile();
}

void RenderPassCreator::CreateDefault(RenderGraph& p_graph) {
    const ivec2 frame_size = DVAR_GET_IVEC2(resolution);
    const int w = frame_size.x;
    const int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frameWidth = w;
    config.frameHeight = h;
    config.enableBloom = false;
    config.enableIbl = false;
    config.enableVxgi = false;
    RenderPassCreator creator(config, p_graph);

    creator.AddShadowPass();
    creator.AddGBufferPass();
    creator.AddLightingPass();
    creator.AddEmitterPass();
    creator.AddBloomPass();
    creator.AddTonePass();

    p_graph.Compile();
}

GpuTextureDesc RenderPassCreator::BuildDefaultTextureDesc(RenderTargetResourceName p_name,
                                                          PixelFormat p_format,
                                                          AttachmentType p_type,
                                                          uint32_t p_width,
                                                          uint32_t p_height) {
    GpuTextureDesc desc{};
    desc.type = p_type;
    desc.name = p_name;
    desc.format = p_format;
    desc.arraySize = 1;
    desc.dimension = Dimension::TEXTURE_2D;
    desc.width = p_width;
    desc.height = p_height;
    desc.mipLevels = 1;
    desc.initialData = nullptr;

    switch (p_type) {
        case AttachmentType::COLOR_2D:
        case AttachmentType::DEPTH_2D:
        case AttachmentType::DEPTH_STENCIL_2D:
        case AttachmentType::SHADOW_2D:
            desc.dimension = Dimension::TEXTURE_2D;
            break;
        case AttachmentType::SHADOW_CUBE_MAP:
        case AttachmentType::COLOR_CUBE_MAP:
            desc.dimension = Dimension::TEXTURE_CUBE;
            desc.miscFlags |= RESOURCE_MISC_TEXTURECUBE;
            desc.arraySize = 6;
            break;
        default:
            CRASH_NOW();
            break;
    }
    switch (p_type) {
        case AttachmentType::COLOR_2D:
        case AttachmentType::COLOR_CUBE_MAP:
            desc.bindFlags |= BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
            break;
        case AttachmentType::SHADOW_2D:
        case AttachmentType::SHADOW_CUBE_MAP:
            desc.bindFlags |= BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
            break;
        case AttachmentType::DEPTH_2D:
        case AttachmentType::DEPTH_STENCIL_2D:
            desc.bindFlags |= BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
            break;
        default:
            break;
    }
    return desc;
};

}  // namespace my::rg
