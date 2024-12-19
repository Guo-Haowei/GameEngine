#include "pass_creator.h"

#include "engine/assets/asset.h"
#include "engine/core/debugger/profiler.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/math/matrix_transform.h"
#include "engine/renderer/draw_data.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/render_graph/render_graph_defines.h"
#include "engine/renderer/renderer_misc.h"

// shader defines
#include "shader_resource_defines.hlsl.h"
#include "unordered_access_defines.hlsl.h"

namespace my::renderer {

/// Gbuffer
static void DrawBatchesGeometry(const DrawData& p_data, const std::vector<BatchContext>& p_batches) {
    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    for (const auto& draw : p_batches) {
        const bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), draw.bone_idx);
        }

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);

        gm.SetMesh(draw.mesh_data);

        if (draw.flags) {
            gm.SetStencilRef(draw.flags);
        }

        for (const auto& subset : draw.subsets) {
            // @TODO: fix this
            const MaterialConstantBuffer& material = p_data.materialCache.buffer[subset.material_idx];
            gm.BindTexture(Dimension::TEXTURE_2D, material.c_baseColorMapHandle, GetBaseColorMapSlot());
            gm.BindTexture(Dimension::TEXTURE_2D, material.c_normalMapHandle, GetNormalMapSlot());
            gm.BindTexture(Dimension::TEXTURE_2D, material.c_materialMapHandle, GetMaterialMapSlot());

            gm.BindConstantBufferSlot<MaterialConstantBuffer>(frame.materialCb.get(), subset.material_idx);

            gm.DrawElements(subset.index_count, subset.index_offset);
        }

        if (draw.flags) {
            gm.SetStencilRef(0);
        }
    }
}

static void GbufferPassFunc(const DrawData& p_data, const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();
    const uint32_t width = p_draw_pass->desc.depthAttachment->desc.width;
    const uint32_t height = p_draw_pass->desc.depthAttachment->desc.height;

    gm.SetRenderTarget(p_draw_pass);
    gm.SetViewport(Viewport(width, height));

    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT, clear_color);

    const PassContext& pass = p_data.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    gm.SetPipelineState(PSO_GBUFFER);
    DrawBatchesGeometry(p_data, pass.opaque);
    gm.SetPipelineState(PSO_GBUFFER_DOUBLE_SIDED);
    DrawBatchesGeometry(p_data, pass.doubleSided);
}

void RenderPassCreator::AddGbufferPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    const int w = m_config.frameWidth;
    const int h = m_config.frameHeight;

    auto gbuffer_depth = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_DEPTH,
                                                                       RESOURCE_FORMAT_GBUFFER_DEPTH,
                                                                       AttachmentType::DEPTH_STENCIL_2D,
                                                                       w, h),
                                               PointClampSampler());

    auto attachment0 = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_BASE_COLOR,
                                                                     RESOURCE_FORMAT_GBUFFER_BASE_COLOR,
                                                                     AttachmentType::COLOR_2D,
                                                                     w, h),
                                             PointClampSampler());

    auto attachment1 = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_POSITION,
                                                                     RESOURCE_FORMAT_GBUFFER_POSITION,
                                                                     AttachmentType::COLOR_2D,
                                                                     w, h),
                                             PointClampSampler());

    auto attachment2 = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_NORMAL,
                                                                     RESOURCE_FORMAT_GBUFFER_NORMAL,
                                                                     AttachmentType::COLOR_2D,
                                                                     w, h),
                                             PointClampSampler());

    auto attachment3 = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_GBUFFER_MATERIAL,
                                                                     RESOURCE_FORMAT_GBUFFER_MATERIAL,
                                                                     AttachmentType::COLOR_2D,
                                                                     w, h),
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

static void HighlightPassFunc(const DrawData&, const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);
    const auto [width, height] = p_draw_pass->GetBufferSize();

    gm.SetViewport(Viewport(width, height));

    gm.SetPipelineState(PSO_HIGHLIGHT);
    gm.SetStencilRef(STENCIL_FLAG_SELECTED);
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);
    gm.DrawQuad();
    gm.SetStencilRef(0);
}

void RenderPassCreator::AddHighlightPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    const int w = m_config.frameWidth;
    const int h = m_config.frameHeight;

    auto gbuffer_depth = manager.FindTexture(RESOURCE_GBUFFER_DEPTH);
    auto attachment = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_HIGHLIGHT_SELECT,
                                                                    RESOURCE_FORMAT_HIGHLIGHT_SELECT,
                                                                    AttachmentType::COLOR_2D,
                                                                    w, h),
                                            PointClampSampler());

    RenderPassDesc desc;
    desc.name = RenderPassName::HIGHLIGHT_SELECT;
    desc.dependencies = { RenderPassName::GBUFFER };
    auto pass = m_graph.CreatePass(desc);
    auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
        .colorAttachments = { attachment },
        .depthAttachment = gbuffer_depth,
        .execFunc = HighlightPassFunc,
    });
    pass->AddDrawPass(draw_pass);
}

/// Shadow
static void PointShadowPassFunc(const DrawData& p_data, const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    // prepare render data
    const auto [width, height] = p_draw_pass->GetBufferSize();

    auto draw_batches = [&](const std::vector<BatchContext>& p_batches) {
        for (const auto& draw : p_batches) {
            const bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                gm.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), draw.bone_idx);
            }

            gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);

            gm.SetMesh(draw.mesh_data);
            gm.DrawElements(draw.mesh_data->desc.drawCount);
        }
    };

    for (int pass_id = 0; pass_id < MAX_POINT_LIGHT_SHADOW_COUNT; ++pass_id) {
        auto& pass_ptr = p_data.pointShadowPasses[pass_id];
        if (!pass_ptr) {
            continue;
        }

        const PassContext& pass = *pass_ptr.get();
        for (int face_id = 0; face_id < 6; ++face_id) {
            const uint32_t slot = pass_id * 6 + face_id;
            gm.BindConstantBufferSlot<PointShadowConstantBuffer>(frame.pointShadowCb.get(), slot);

            gm.SetRenderTarget(p_draw_pass, slot);
            gm.Clear(p_draw_pass, CLEAR_DEPTH_BIT, nullptr, slot);

            gm.SetViewport(Viewport(width, height));

            gm.SetPipelineState(PSO_POINT_SHADOW);
            draw_batches(pass.opaque);
            draw_batches(pass.transparent);
            // @TODO: double side
            draw_batches(pass.doubleSided);
        }
    }
}

static void ShadowPassFunc(const DrawData& p_data, const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    gm.SetRenderTarget(p_draw_pass);
    const auto [width, height] = p_draw_pass->GetBufferSize();

    gm.Clear(p_draw_pass, CLEAR_DEPTH_BIT);

    gm.SetViewport(Viewport(width, height));

    const PassContext& pass = p_data.shadowPasses[0];
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    auto draw_batches = [&](const std::vector<BatchContext>& p_batches) {
        for (const auto& draw : p_batches) {
            const bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                gm.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), draw.bone_idx);
            }

            gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);

            gm.SetMesh(draw.mesh_data);
            gm.DrawElements(draw.mesh_data->desc.drawCount);
        }
    };
    gm.SetPipelineState(PSO_DPETH);
    draw_batches(pass.opaque);
    draw_batches(pass.transparent);
    draw_batches(pass.doubleSided);
}

void RenderPassCreator::AddShadowPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    const int shadow_res = DVAR_GET_INT(gfx_shadow_res);
    DEV_ASSERT(math::IsPowerOfTwo(shadow_res));
    const int point_shadow_res = DVAR_GET_INT(gfx_point_shadow_res);
    DEV_ASSERT(math::IsPowerOfTwo(point_shadow_res));

    auto shadow_map = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_SHADOW_MAP,
                                                                    PixelFormat::D32_FLOAT,
                                                                    AttachmentType::SHADOW_2D,
                                                                    1 * shadow_res, shadow_res),
                                            shadow_map_sampler());
    RenderPassDesc desc;
    desc.name = RenderPassName::SHADOW;
    desc.dependencies = { RenderPassName::ENV };
    auto pass = m_graph.CreatePass(desc);
    {
        auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
            .depthAttachment = shadow_map,
            .execFunc = ShadowPassFunc,
        });
        pass->AddDrawPass(draw_pass);
    }

    auto point_shadowMap = manager.CreateTexture(BuildDefaultTextureDesc(static_cast<RenderTargetResourceName>(RESOURCE_POINT_SHADOW_CUBE_ARRAY),
                                                                         PixelFormat::D32_FLOAT,
                                                                         AttachmentType::SHADOW_CUBE_ARRAY,
                                                                         point_shadow_res, point_shadow_res, 6 * MAX_POINT_LIGHT_SHADOW_COUNT),
                                                 shadow_cube_map_sampler());

    auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
        .depthAttachment = point_shadowMap,
        .execFunc = PointShadowPassFunc,
    });
    pass->AddDrawPass(draw_pass);
}

static void VoxelizationPassFunc(const DrawData& p_data, const DrawPass*) {
    OPTICK_EVENT();
    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    if (!DVAR_GET_BOOL(gfx_enable_vxgi)) {
        return;
    }

    auto voxel_lighting = gm.FindTexture(RESOURCE_VOXEL_LIGHTING);
    auto voxel_normal = gm.FindTexture(RESOURCE_VOXEL_NORMAL);
    DEV_ASSERT(voxel_lighting && voxel_normal);

    // @TODO: refactor pass to auto bind resources,
    // and make it a class so don't do a map search every frame
    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<GpuTexture> resource = gm.FindTexture(p_name);
        if (!resource) {
            return;
        }

        gm.BindTexture(p_dimension, resource->GetHandle(), p_slot);
    };

    // bind common textures
    bind_slot(RESOURCE_SHADOW_MAP, GetShadowMapSlot());
    bind_slot(RESOURCE_POINT_SHADOW_CUBE_ARRAY, GetPointShadowArraySlot(), Dimension::TEXTURE_CUBE_ARRAY);

    const int voxel_size = DVAR_GET_INT(gfx_voxel_size);

    gm.BindUnorderedAccessView(IMAGE_VOXEL_ALBEDO_SLOT, voxel_lighting.get());
    gm.BindUnorderedAccessView(IMAGE_VOXEL_NORMAL_SLOT, voxel_normal.get());

    // post process
    const uint32_t group_size = voxel_size / COMPUTE_LOCAL_SIZE_VOXEL;
    gm.SetPipelineState(PSO_VOXELIZATION_PRE);
    gm.Dispatch(group_size, group_size, group_size);

    const PassContext& pass = p_data.voxelPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    // glSubpixelPrecisionBiasNV(1, 1);
    // glSubpixelPrecisionBiasNV(8, 8);

    // @TODO: hack
    if (gm.GetBackend() == Backend::OPENGL) {
        auto draw_batches = [&](const std::vector<BatchContext>& p_batches) {
            for (const auto& draw : p_batches) {
                const bool has_bone = draw.bone_idx >= 0;
                if (has_bone) {
                    gm.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), draw.bone_idx);
                }

                gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);

                gm.SetMesh(draw.mesh_data);

                for (const auto& subset : draw.subsets) {
                    gm.BindConstantBufferSlot<MaterialConstantBuffer>(frame.materialCb.get(), subset.material_idx);

                    gm.DrawElements(subset.index_count, subset.index_offset);
                }
            }
        };

        gm.SetViewport(Viewport(voxel_size, voxel_size));
        gm.SetPipelineState(PSO_VOXELIZATION);
        gm.SetBlendState(PipelineStateManager::GetBlendDescDisable(), nullptr, 0xFFFFFFFF);
        draw_batches(pass.opaque);
        draw_batches(pass.transparent);
        draw_batches(pass.doubleSided);

        // glSubpixelPrecisionBiasNV(0, 0);
        gm.SetBlendState(PipelineStateManager::GetBlendDescDefault(), nullptr, 0xFFFFFFFF);
    }

    // post process
    gm.SetPipelineState(PSO_VOXELIZATION_POST);
    gm.Dispatch(group_size, group_size, group_size);

    gm.GenerateMipmap(voxel_lighting.get());
    gm.GenerateMipmap(voxel_normal.get());

    // unbind stuff
    gm.UnbindTexture(Dimension::TEXTURE_2D, GetShadowMapSlot());
    gm.UnbindTexture(Dimension::TEXTURE_CUBE_ARRAY, GetPointShadowArraySlot());

    // @TODO: [SCRUM-28] refactor
    gm.UnsetRenderTarget();
}

void RenderPassCreator::AddVoxelizationPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    {
        const int voxel_size = DVAR_GET_INT(gfx_voxel_size);
        GpuTextureDesc desc = BuildDefaultTextureDesc(RESOURCE_VOXEL_LIGHTING,
                                                      PixelFormat::R16G16B16A16_FLOAT,
                                                      AttachmentType::RW_TEXTURE,
                                                      voxel_size, voxel_size);
        desc.dimension = Dimension::TEXTURE_3D;
        desc.mipLevels = math::LogTwo(voxel_size);
        desc.depth = voxel_size;
        desc.miscFlags |= RESOURCE_MISC_GENERATE_MIPS;
        desc.bindFlags |= BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

        SamplerDesc sampler(FilterMode::MIPMAP_LINEAR, FilterMode::POINT, AddressMode::BORDER);

        auto voxel_lighting = manager.CreateTexture(desc, sampler);

        desc.name = RESOURCE_VOXEL_NORMAL;
        auto voxel_normal = manager.CreateTexture(desc, sampler);
    }

    RenderPassDesc desc;
    desc.name = RenderPassName::VOXELIZATION;
    desc.dependencies = { RenderPassName::SHADOW };
    auto pass = m_graph.CreatePass(desc);
    auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
        .execFunc = VoxelizationPassFunc,
    });
    pass->AddDrawPass(draw_pass);
}

/// Lighting
static void LightingPassFunc(const DrawData& p_data, const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    const auto [width, height] = p_draw_pass->GetBufferSize();

    gm.SetRenderTarget(p_draw_pass);

    gm.SetViewport(Viewport(width, height));
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);
    gm.SetPipelineState(PSO_LIGHTING);

    const auto brdf = gm.m_brdfImage->gpu_texture;
    auto diffuse_iraddiance = gm.FindTexture(RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP);
    auto prefiltered = gm.FindTexture(RESOURCE_ENV_PREFILTER_CUBE_MAP);
    DEV_ASSERT(brdf && diffuse_iraddiance && prefiltered);

    // @TODO: fix
    auto voxel_lighting = gm.FindTexture(RESOURCE_VOXEL_LIGHTING);
    auto voxel_normal = gm.FindTexture(RESOURCE_VOXEL_NORMAL);
    if (voxel_lighting && voxel_normal) {
        gm.BindTexture(Dimension::TEXTURE_3D, voxel_lighting->GetHandle(), GetVoxelLightingSlot());
        gm.BindTexture(Dimension::TEXTURE_3D, voxel_normal->GetHandle(), GetVoxelNormalSlot());
    }

    gm.BindTexture(Dimension::TEXTURE_2D, brdf->GetHandle(), GetBrdfLutSlot());
    gm.BindTexture(Dimension::TEXTURE_CUBE, diffuse_iraddiance->GetHandle(), GetDiffuseIrradianceSlot());
    gm.BindTexture(Dimension::TEXTURE_CUBE, prefiltered->GetHandle(), GetPrefilteredSlot());

    gm.DrawQuad();

    gm.UnbindTexture(Dimension::TEXTURE_2D, GetBrdfLutSlot());
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, GetDiffuseIrradianceSlot());
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, GetPrefilteredSlot());

    if (voxel_lighting && voxel_normal) {
        gm.UnbindTexture(Dimension::TEXTURE_3D, GetVoxelLightingSlot());
        gm.UnbindTexture(Dimension::TEXTURE_3D, GetVoxelNormalSlot());
    }

    const PassContext& pass = p_data.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.GetCurrentFrame().passCb.get(), pass.pass_idx);

    // DrawBatches
    auto skybox = p_data.skyboxHdr;
    if (skybox) {
        gm.BindTexture(Dimension::TEXTURE_2D, skybox->GetHandle(), GetSkyboxHdrSlot());
        gm.SetPipelineState(PSO_ENV_SKYBOX);
        gm.DrawSkybox();
        gm.UnbindTexture(Dimension::TEXTURE_2D, GetSkyboxHdrSlot());
    }

    // draw transparent objects
    gm.SetPipelineState(PSO_FORWARD_TRANSPARENT);
    DrawBatchesGeometry(p_data, pass.transparent);

    auto& draw_context = p_data.drawDebugContext;
    if (gm.m_debugBuffers && draw_context.drawCount) {
        gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.GetCurrentFrame().passCb.get(), pass.pass_idx);
        gm.SetPipelineState(PSO_DEBUG_DRAW);

        gm.UpdateBuffer(renderer::CreateDesc(draw_context.positions),
                        gm.m_debugBuffers->vertexBuffers[0].get());
        gm.UpdateBuffer(renderer::CreateDesc(draw_context.colors),
                        gm.m_debugBuffers->vertexBuffers[6].get());
        gm.SetMesh(gm.m_debugBuffers.get());
        gm.DrawArrays(draw_context.drawCount);
    }
}

void RenderPassCreator::AddLightingPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    auto gbuffer_depth = manager.FindTexture(RESOURCE_GBUFFER_DEPTH);

    auto lighting_attachment = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_LIGHTING,
                                                                             RESOURCE_FORMAT_LIGHTING,
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
    auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
        .colorAttachments = { lighting_attachment },
        .depthAttachment = gbuffer_depth,
        .transitions = {
            ResourceTransition{
                .resource = manager.FindTexture(RESOURCE_SHADOW_MAP),
                .slot = GetShadowMapSlot(),
                .beginPassFunc = [](GraphicsManager* p_graphics_manager,
                                    GpuTexture* p_texture,
                                    int p_slot) { p_graphics_manager->BindTexture(Dimension::TEXTURE_2D, p_texture->GetHandle(), p_slot); },
                .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                  GpuTexture*,
                                  int p_slot) { p_graphics_manager->UnbindTexture(Dimension::TEXTURE_2D, p_slot); },
            },
            ResourceTransition{
                .resource = manager.FindTexture(RESOURCE_POINT_SHADOW_CUBE_ARRAY),
                .slot = GetPointShadowArraySlot(),
                .beginPassFunc = [](GraphicsManager* p_graphics_manager,
                                    GpuTexture* p_texture,
                                    int p_slot) { p_graphics_manager->BindTexture(Dimension::TEXTURE_CUBE_ARRAY, p_texture->GetHandle(), p_slot); },
                .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                  GpuTexture*,
                                  int p_slot) { p_graphics_manager->UnbindTexture(Dimension::TEXTURE_CUBE_ARRAY, p_slot); },
            },
        },
        .execFunc = LightingPassFunc,
    });
    pass->AddDrawPass(draw_pass);
}

/// Emitter
static void EmitterPassFunc(const DrawData& p_data, const DrawPass* p_draw_pass) {
    unused(p_data);
    unused(p_draw_pass);
#if 0
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();
    const auto [width, height] = p_draw_pass->GetBufferSize();

    gm.SetRenderTarget(p_draw_pass);
    gm.SetViewport(Viewport(width, height));

    const PassContext& pass = p_data.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    const Scene& scene = SceneManager::GetScene();
    int particle_idx = 0;
    for (auto [id, emitter] : scene.m_ParticleEmitterComponents) {
        gm.BindConstantBufferSlot<EmitterConstantBuffer>(frame.emitterCb.get(), particle_idx++);

        gm.BindStructuredBuffer(GetGlobalParticleCounterSlot(), emitter.counterBuffer.get());
        gm.BindStructuredBuffer(GetGlobalDeadIndicesSlot(), emitter.deadBuffer.get());
        gm.BindStructuredBuffer(GetGlobalAliveIndicesPreSimSlot(), emitter.aliveBuffer[emitter.GetPreIndex()].get());
        gm.BindStructuredBuffer(GetGlobalAliveIndicesPostSimSlot(), emitter.aliveBuffer[emitter.GetPostIndex()].get());
        gm.BindStructuredBuffer(GetGlobalParticleDataSlot(), emitter.particleBuffer.get());

        gm.SetPipelineState(PSO_PARTICLE_KICKOFF);
        gm.Dispatch(1, 1, 1);

        gm.SetPipelineState(PSO_PARTICLE_EMIT);
        gm.Dispatch(MAX_PARTICLE_COUNT / PARTICLE_LOCAL_SIZE, 1, 1);

        gm.SetPipelineState(PSO_PARTICLE_SIM);
        gm.Dispatch(MAX_PARTICLE_COUNT / PARTICLE_LOCAL_SIZE, 1, 1);

        gm.UnbindStructuredBuffer(GetGlobalParticleCounterSlot());
        gm.UnbindStructuredBuffer(GetGlobalDeadIndicesSlot());
        gm.UnbindStructuredBuffer(GetGlobalAliveIndicesPreSimSlot());
        gm.UnbindStructuredBuffer(GetGlobalAliveIndicesPostSimSlot());
        gm.UnbindStructuredBuffer(GetGlobalParticleDataSlot());

        // Renderering
        gm.SetPipelineState(PSO_PARTICLE_RENDERING);

        gm.BindStructuredBufferSRV(GetGlobalParticleDataSlot(), emitter.particleBuffer.get());
        RenderManager::GetSingleton().draw_quad_instanced(MAX_PARTICLE_COUNT);
        gm.UnbindStructuredBufferSRV(GetGlobalParticleDataSlot());
    }
#endif
}

void RenderPassCreator::AddEmitterPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::EMITTER;

    desc.dependencies = { RenderPassName::LIGHTING };

    auto pass = m_graph.CreatePass(desc);
    auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
        .colorAttachments = { manager.FindTexture(RESOURCE_LIGHTING) },
        .depthAttachment = manager.FindTexture(RESOURCE_GBUFFER_DEPTH),
        .execFunc = EmitterPassFunc,
    });
    pass->AddDrawPass(draw_pass);
}

/// Bloom
static void BloomSetupFunc(const DrawData&, const DrawPass* p_draw_pass) {
    unused(p_draw_pass);

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    gm.SetPipelineState(PSO_BLOOM_SETUP);
    auto input = gm.FindTexture(RESOURCE_LIGHTING);

    const uint32_t width = input->desc.width;
    const uint32_t height = input->desc.height;
    const uint32_t work_group_x = math::CeilingDivision(width, 16);
    const uint32_t work_group_y = math::CeilingDivision(height, 16);

    gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), 0);

    gm.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), GetBloomInputTextureSlot());
    gm.Dispatch(work_group_x, work_group_y, 1);
    gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

static void BloomDownSampleFunc(const DrawData&, const DrawPass* p_draw_pass) {
    const uint32_t pass_id = p_draw_pass->id;
    GraphicsManager& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    gm.SetPipelineState(PSO_BLOOM_DOWNSAMPLE);
    auto input = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + pass_id));

    DEV_ASSERT(input);

    const uint32_t width = input->desc.width;
    const uint32_t height = input->desc.height;
    const uint32_t work_group_x = math::CeilingDivision(width, 16);
    const uint32_t work_group_y = math::CeilingDivision(height, 16);

    gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), pass_id + 1);

    gm.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), GetBloomInputTextureSlot());
    gm.Dispatch(work_group_x, work_group_y, 1);
    gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

static void BloomUpSampleFunc(const DrawData&, const DrawPass* p_draw_pass) {
    GraphicsManager& gm = GraphicsManager::GetSingleton();
    const uint32_t pass_id = p_draw_pass->id;
    auto& frame = gm.GetCurrentFrame();

    gm.SetPipelineState(PSO_BLOOM_UPSAMPLE);
    auto input = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + pass_id));
    auto output = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + pass_id - 1));

    const uint32_t width = output->desc.width;
    const uint32_t height = output->desc.height;
    const uint32_t work_group_x = math::CeilingDivision(width, 16);
    const uint32_t work_group_y = math::CeilingDivision(height, 16);

    gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), pass_id + BLOOM_MIP_CHAIN_MAX);

    gm.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), GetBloomInputTextureSlot());
    gm.Dispatch(work_group_x, work_group_y, 1);
    gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

void RenderPassCreator::AddBloomPass() {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::BLOOM;
    desc.dependencies = { RenderPassName::LIGHTING };
    auto render_pass = m_graph.CreatePass(desc);

    int width = m_config.frameWidth;
    int height = m_config.frameHeight;
    for (int i = 0; i < BLOOM_MIP_CHAIN_MAX; ++i, width /= 2, height /= 2) {
        DEV_ASSERT(width > 1);
        DEV_ASSERT(height > 1);

        LOG_WARN("bloom size {}x{}", width, height);

        GpuTextureDesc texture_desc = BuildDefaultTextureDesc(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i),
                                                              PixelFormat::R16G16B16A16_FLOAT,
                                                              AttachmentType::COLOR_2D,
                                                              width, height);
        texture_desc.bindFlags |= BIND_UNORDERED_ACCESS;
        auto attachment = gm.CreateTexture(texture_desc, LinearClampSampler());
    }

    // Setup
    {
        auto output = gm.FindTexture(RESOURCE_BLOOM_0);
        DEV_ASSERT(output);
        auto pass = gm.CreateDrawPass(DrawPassDesc{
            .transitions = {
                ResourceTransition{
                    .resource = output,
                    .slot = GetUavSlotBloomOutputImage(),
                    .beginPassFunc = [](GraphicsManager* p_graphics_manager,
                                        GpuTexture* p_texture,
                                        int p_slot) { p_graphics_manager->BindUnorderedAccessView(p_slot, p_texture); },
                    .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                      GpuTexture*,
                                      int p_slot) { p_graphics_manager->UnbindUnorderedAccessView(p_slot); },
                } },
            .execFunc = BloomSetupFunc,
        });
        render_pass->AddDrawPass(pass);
    }

    // Down Sample
    for (int i = 0; i < BLOOM_MIP_CHAIN_MAX - 1; ++i) {
        auto output = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i + 1));
        DEV_ASSERT(output);
        auto pass = gm.CreateDrawPass(DrawPassDesc{
            .transitions = {
                ResourceTransition{
                    .resource = output,
                    .slot = GetUavSlotBloomOutputImage(),
                    .beginPassFunc = [](GraphicsManager* p_graphics_manager,
                                        GpuTexture* p_texture,
                                        int p_slot) { p_graphics_manager->BindUnorderedAccessView(p_slot, p_texture); },
                    .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                      GpuTexture*,
                                      int p_slot) { p_graphics_manager->UnbindUnorderedAccessView(p_slot); },
                } },
            .execFunc = BloomDownSampleFunc,
        });
        pass->id = i;
        render_pass->AddDrawPass(pass);
    }

    // Up Sample
    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto output = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));
        DEV_ASSERT(output);
        auto pass = gm.CreateDrawPass(DrawPassDesc{
            .transitions = {
                ResourceTransition{
                    .resource = output,
                    .slot = GetUavSlotBloomOutputImage(),
                    .beginPassFunc = [](GraphicsManager* p_graphics_manager,
                                        GpuTexture* p_texture,
                                        int p_slot) { p_graphics_manager->BindUnorderedAccessView(p_slot, p_texture); },
                    .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                      GpuTexture*,
                                      int p_slot) { p_graphics_manager->UnbindUnorderedAccessView(p_slot); },
                } },
            .execFunc = BloomUpSampleFunc,
        });
        pass->id = i;
        render_pass->AddDrawPass(pass);
    }
}

/// Tone
/// Change to post processing?
static void TonePassFunc(const DrawData& p_data, const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);

    auto depth_buffer = p_draw_pass->desc.depthAttachment;
    const auto [width, height] = p_draw_pass->GetBufferSize();

    // draw billboards

    // @HACK:
    if (DVAR_GET_BOOL(gfx_debug_vxgi) && gm.GetBackend() == Backend::OPENGL) {
        // @TODO: remove
        extern void debug_vxgi_pass_func(const DrawData&, const DrawPass* p_draw_pass);
        debug_vxgi_pass_func(p_data, p_draw_pass);
    } else {
        auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
            std::shared_ptr<GpuTexture> resource = gm.FindTexture(p_name);
            if (!resource) {
                return;
            }

            gm.BindTexture(p_dimension, resource->GetHandle(), p_slot);
        };
        bind_slot(RESOURCE_LIGHTING, GetTextureLightingSlot());
        bind_slot(RESOURCE_HIGHLIGHT_SELECT, GetTextureHighlightSelectSlot());
        bind_slot(RESOURCE_BLOOM_0, GetBloomInputTextureSlot());

        gm.SetViewport(Viewport(width, height));
        gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);

        gm.SetPipelineState(PSO_TONE);
        gm.DrawQuad();

        gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
        gm.UnbindTexture(Dimension::TEXTURE_2D, GetTextureLightingSlot());
        gm.UnbindTexture(Dimension::TEXTURE_2D, GetTextureHighlightSelectSlot());
    }
}

void RenderPassCreator::AddTonePass() {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::TONE;

    if (m_config.enableBloom) {
        desc.dependencies = { RenderPassName::BLOOM };
    }

    auto pass = m_graph.CreatePass(desc);

    const int width = m_config.frameWidth;
    const int height = m_config.frameHeight;

    auto attachment = gm.FindTexture(RESOURCE_TONE);

    if (!attachment) {
        attachment = gm.CreateTexture(BuildDefaultTextureDesc(RESOURCE_TONE,
                                                              RESOURCE_FORMAT_TONE,
                                                              AttachmentType::COLOR_2D,
                                                              width, height),
                                      PointClampSampler());
    }

    DrawPassDesc draw_pass_desc{
        .colorAttachments = { attachment },
        .execFunc = TonePassFunc,
    };

    if (m_config.enableHighlight) {
        auto gbuffer_depth = gm.FindTexture(RESOURCE_GBUFFER_DEPTH);
        draw_pass_desc.depthAttachment = gbuffer_depth;
    }

    auto draw_pass = gm.CreateDrawPass(draw_pass_desc);

    pass->AddDrawPass(draw_pass);
}

void RenderPassCreator::AddDebugImagePass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

    auto final_attachment = manager.FindTexture(RESOURCE_TONE);

    // final pass
    RenderPassDesc desc;
    desc.name = RenderPassName::FINAL;
    desc.dependencies = { RenderPassName::TONE };
    auto pass = m_graph.CreatePass(desc);
    auto draw_pass = manager.CreateDrawPass(DrawPassDesc{
        .colorAttachments = { final_attachment },
        .execFunc = [](const DrawData& p_data, const DrawPass* p_draw_pass) {
            OPTICK_EVENT();

            auto& gm = GraphicsManager::GetSingleton();
            auto& frame = gm.GetCurrentFrame();
            const uint32_t width = p_draw_pass->desc.colorAttachments[0]->desc.width;
            const uint32_t height = p_draw_pass->desc.colorAttachments[0]->desc.height;

            gm.SetRenderTarget(p_draw_pass);
            gm.SetViewport(Viewport(width, height));
            gm.SetPipelineState(PSO_RW_TEXTURE_2D);

            for (int i = 0; i < (int)p_data.drawImageContext.size(); ++i) {
                const auto& data = p_data.drawImageContext[i];
                gm.BindTexture(Dimension::TEXTURE_2D, data.handle, GetBaseColorMapSlot());
                gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
                gm.DrawQuad();
                gm.UnbindTexture(Dimension::TEXTURE_2D, GetBaseColorMapSlot());
            }
        } });
    pass->AddDrawPass(draw_pass);
}

void RenderPassCreator::AddGenerateSkylightPass() {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    RenderPassDesc render_pass_desc{
        .name = RenderPassName::ENV,
    };
    // @TODO: rename to skylight
    auto render_pass = m_graph.CreatePass(render_pass_desc);

    // hdr -> cubemap
    {
        const int size = 512;
        auto desc = BuildDefaultTextureDesc(RESOURCE_ENV_SKYBOX_CUBE_MAP,
                                            PixelFormat::R16G16B16A16_FLOAT,
                                            AttachmentType::COLOR_CUBE,
                                            size, size, 6);
        auto cubemap = gm.CreateTexture(desc, SamplerDesc(FilterMode::LINEAR,
                                                          FilterMode::LINEAR,
                                                          AddressMode::CLAMP));
        DEV_ASSERT(cubemap);
        auto pass = gm.CreateDrawPass(DrawPassDesc{
            .colorAttachments = { cubemap },
            .execFunc = [](const DrawData& p_data, const DrawPass* p_draw_pass) {
                if (!p_data.bakeIbl) {
                    return;
                }
                OPTICK_EVENT("hdr image to -> skybox");

                GraphicsManager& gm = GraphicsManager::GetSingleton();

                gm.SetPipelineState(PSO_ENV_SKYBOX_TO_CUBE_MAP);
                auto cube_map = p_draw_pass->desc.colorAttachments[0];
                const auto [width, height] = p_draw_pass->GetBufferSize();

                auto& frame = gm.GetCurrentFrame();
                gm.BindTexture(Dimension::TEXTURE_2D, p_data.skyboxHdr->GetHandle(), GetSkyboxHdrSlot());
                for (int i = 0; i < 6; ++i) {
                    gm.SetRenderTarget(p_draw_pass, i);

                    gm.SetViewport(Viewport(width, height));

                    gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
                    gm.DrawSkybox();
                }
                gm.UnbindTexture(Dimension::TEXTURE_2D, GetSkyboxHdrSlot());

                gm.GenerateMipmap(cube_map.get());
            },
        });
        render_pass->AddDrawPass(pass);
    }
    // Diffuse irradiance
    {
        const int size = 32;
        auto desc = BuildDefaultTextureDesc(RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP,
                                            PixelFormat::R16G16B16A16_FLOAT,
                                            AttachmentType::COLOR_CUBE,
                                            size, size, 6);
        auto cubemap = gm.CreateTexture(desc, LinearClampSampler());
        DEV_ASSERT(cubemap);

        auto pass = gm.CreateDrawPass(DrawPassDesc{
            .colorAttachments = { cubemap },
            .execFunc = [](const DrawData& p_data, const DrawPass* p_draw_pass) {
                if (!p_data.bakeIbl) {
                    return;
                }
                OPTICK_EVENT("bake diffuse irradiance");

                GraphicsManager& gm = GraphicsManager::GetSingleton();

                gm.SetPipelineState(PSO_DIFFUSE_IRRADIANCE);
                const auto [width, height] = p_draw_pass->GetBufferSize();

                auto skybox = gm.FindTexture(RESOURCE_ENV_SKYBOX_CUBE_MAP);
                DEV_ASSERT(skybox);
                gm.BindTexture(Dimension::TEXTURE_CUBE, skybox->GetHandle(), GetSkyboxSlot());
                auto& frame = gm.GetCurrentFrame();
                for (int i = 0; i < 6; ++i) {
                    gm.SetRenderTarget(p_draw_pass, i);
                    gm.SetViewport(Viewport(width, height));

                    gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
                    gm.DrawSkybox();
                }
                gm.UnbindTexture(Dimension::TEXTURE_CUBE, GetSkyboxSlot());
            },
        });
        render_pass->AddDrawPass(pass);
    }
    // prefilter
    {
        const int size = 512;
        auto desc = BuildDefaultTextureDesc(RESOURCE_ENV_PREFILTER_CUBE_MAP,
                                            PixelFormat::R16G16B16A16_FLOAT,
                                            AttachmentType::COLOR_CUBE,
                                            size, size, 6);
        if (gm.GetBackend() == Backend::OPENGL) {
            // @HACK
            desc.miscFlags |= RESOURCE_MISC_GENERATE_MIPS;
        }
        desc.mipLevels = IBL_MIP_CHAIN_MAX;

        auto cubemap = gm.CreateTexture(desc, SamplerDesc(FilterMode::LINEAR,
                                                          FilterMode::LINEAR,
                                                          AddressMode::CLAMP));
        DEV_ASSERT(cubemap);

        auto pass = gm.CreateDrawPass(DrawPassDesc{
            .colorAttachments = { cubemap },
            .execFunc = [](const DrawData& p_data, const DrawPass* p_draw_pass) {
                if (!p_data.bakeIbl) {
                    return;
                }
                OPTICK_EVENT("bake prefiltered");

                GraphicsManager& gm = GraphicsManager::GetSingleton();
                gm.SetPipelineState(PSO_PREFILTER);
                auto [width, height] = p_draw_pass->GetBufferSize();

                auto skybox = gm.FindTexture(RESOURCE_ENV_SKYBOX_CUBE_MAP);
                DEV_ASSERT(skybox);
                gm.BindTexture(Dimension::TEXTURE_CUBE, skybox->GetHandle(), GetSkyboxSlot());
                auto& frame = gm.GetCurrentFrame();
                for (int mip_idx = 0; mip_idx < IBL_MIP_CHAIN_MAX; ++mip_idx, width /= 2, height /= 2) {
                    for (int face_id = 0; face_id < 6; ++face_id) {
                        const int index = mip_idx * 6 + face_id;
                        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), index);

                        gm.SetRenderTarget(p_draw_pass, face_id, mip_idx);
                        gm.SetViewport(Viewport(width, height));
                        gm.DrawSkybox();
                    }
                }
                gm.UnbindTexture(Dimension::TEXTURE_CUBE, GetSkyboxSlot());
            },
        });
        render_pass->AddDrawPass(pass);
    }
}

static void PathTracerTonePassFunc(const DrawData&, const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);

    auto depth_buffer = p_draw_pass->desc.depthAttachment;
    const auto [width, height] = p_draw_pass->GetBufferSize();

    // draw billboards

    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<GpuTexture> resource = gm.FindTexture(p_name);
        if (!resource) {
            return;
        }

        gm.BindTexture(p_dimension, resource->GetHandle(), p_slot);
    };
    bind_slot(RESOURCE_PATH_TRACER, GetBloomInputTextureSlot());

    gm.SetViewport(Viewport(width, height));
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);

    gm.SetPipelineState(PSO_TONE);
    gm.DrawQuad();

    gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

void RenderPassCreator::AddPathTracerTonePass() {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::TONE;

    desc.dependencies = { RenderPassName::PATH_TRACER };

    auto pass = m_graph.CreatePass(desc);

    int width = m_config.frameWidth;
    int height = m_config.frameHeight;

    auto attachment = gm.FindTexture(RESOURCE_TONE);

    if (!attachment) {
        attachment = gm.CreateTexture(BuildDefaultTextureDesc(RESOURCE_TONE,
                                                              RESOURCE_FORMAT_TONE,
                                                              AttachmentType::COLOR_2D,
                                                              width, height),
                                      PointClampSampler());
    }

    DrawPassDesc draw_pass_desc{
        .colorAttachments = { attachment },
        .depthAttachment = nullptr,
        .execFunc = PathTracerTonePassFunc,
    };

    if (m_config.enableHighlight) {
        auto gbuffer_depth = gm.FindTexture(RESOURCE_GBUFFER_DEPTH);
        draw_pass_desc.depthAttachment = gbuffer_depth;
    }

    auto draw_pass = gm.CreateDrawPass(draw_pass_desc);

    pass->AddDrawPass(draw_pass);
}

static void PathTracerPassFunc(const DrawData&, const DrawPass*) {
    GraphicsManager& gm = GraphicsManager::GetSingleton();
    if (gm.m_bufferUpdated && gm.m_pathTracerGeometryBuffer) {
        LOG_FATAL("currently broken, cause SwapBuffers crash");
        gm.m_bufferUpdated = false;
        return;
    }
    if (!gm.m_pathTracerGeometryBuffer) {
        return;
    }

    gm.SetPipelineState(PSO_PATH_TRACER);
    auto input = gm.FindTexture(RESOURCE_PATH_TRACER);

    DEV_ASSERT(input);

    const uint32_t width = input->desc.width;
    const uint32_t height = input->desc.height;
    const uint32_t work_group_x = math::CeilingDivision(width, 16);
    const uint32_t work_group_y = math::CeilingDivision(height, 16);

    gm.BindStructuredBuffer(GetGlobalBvhsSlot(), gm.m_pathTracerBvhBuffer.get());
    gm.BindStructuredBuffer(GetGlobalGeometriesSlot(), gm.m_pathTracerGeometryBuffer.get());
    gm.BindStructuredBuffer(GetGlobalMaterialsSlot(), gm.m_pathTracerMaterialBuffer.get());
    gm.Dispatch(work_group_x, work_group_y, 1);
    gm.UnbindStructuredBuffer(GetGlobalBvhsSlot());
    gm.UnbindStructuredBuffer(GetGlobalGeometriesSlot());
    gm.UnbindStructuredBuffer(GetGlobalMaterialsSlot());
}

void RenderPassCreator::AddPathTracerPass() {
    GraphicsManager& gm = GraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::PATH_TRACER;
    desc.dependencies = {};
    auto render_pass = m_graph.CreatePass(desc);

    const int width = m_config.frameWidth;
    const int height = m_config.frameHeight;

    GpuTextureDesc texture_desc = BuildDefaultTextureDesc(RESOURCE_PATH_TRACER,
                                                          PixelFormat::R32G32B32A32_FLOAT,
                                                          AttachmentType::COLOR_2D,
                                                          width, height);
    texture_desc.bindFlags |= BIND_UNORDERED_ACCESS;

    auto resource = gm.CreateTexture(texture_desc, LinearClampSampler());

    auto pass = gm.CreateDrawPass(DrawPassDesc{
        .transitions = {
            ResourceTransition{
                .resource = resource,
                .slot = GetUavSlotPathTracerOutputImage(),
                .beginPassFunc = [](GraphicsManager* p_graphics_manager,
                                    GpuTexture* p_texture,
                                    int p_slot) { p_graphics_manager->BindUnorderedAccessView(p_slot, p_texture); },
                .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                  GpuTexture*,
                                  int p_slot) { p_graphics_manager->UnbindUnorderedAccessView(p_slot); },
            } },
        .execFunc = PathTracerPassFunc,
    });
    render_pass->AddDrawPass(pass);
}

/// Create pre-defined passes
std::unique_ptr<RenderGraph> RenderPassCreator::CreateDummy() {
    const NewVector2i frame_size = DVAR_GET_IVEC2(resolution);
    const int w = frame_size.x;
    const int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frameWidth = w;
    config.frameHeight = h;
    config.enableBloom = false;
    config.enableIbl = false;
    config.enableVxgi = false;

    auto graph = std::make_unique<RenderGraph>();
    RenderPassCreator creator(config, *graph.get());

    creator.AddGbufferPass();

    graph->Compile();
    return graph;
}

std::unique_ptr<RenderGraph> RenderPassCreator::CreatePathTracer() {
    const NewVector2i frame_size = DVAR_GET_IVEC2(resolution);
    const int w = frame_size.x;
    const int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frameWidth = w;
    config.frameHeight = h;
    config.enableBloom = false;
    config.enableIbl = false;
    config.enableVxgi = false;
    config.enableHighlight = false;

    auto graph = std::make_unique<RenderGraph>();
    RenderPassCreator creator(config, *graph.get());

    creator.AddPathTracerPass();
    creator.AddPathTracerTonePass();

    graph->Compile();
    return graph;
}

std::unique_ptr<RenderGraph> RenderPassCreator::CreateDefault() {
    const NewVector2i frame_size = DVAR_GET_IVEC2(resolution);
    const int w = frame_size.x;
    const int h = frame_size.y;

    RenderPassCreator::Config config;
    config.frameWidth = w;
    config.frameHeight = h;
    config.enableBloom = true;
    config.enableIbl = false;
    config.enableVxgi = false;

    auto graph = std::make_unique<RenderGraph>();
    RenderPassCreator creator(config, *graph.get());

    creator.AddGenerateSkylightPass();
    creator.AddShadowPass();
    creator.AddGbufferPass();
    creator.AddHighlightPass();
    // creator.AddVoxelizationPass();
    creator.AddLightingPass();
    creator.AddEmitterPass();
    creator.AddBloomPass();
    creator.AddTonePass();
    creator.AddDebugImagePass();

    graph->Compile();
    return graph;
}

GpuTextureDesc RenderPassCreator::BuildDefaultTextureDesc(RenderTargetResourceName p_name,
                                                          PixelFormat p_format,
                                                          AttachmentType p_type,
                                                          uint32_t p_width,
                                                          uint32_t p_height,
                                                          uint32_t p_array_size) {
    GpuTextureDesc desc{};
    desc.type = p_type;
    desc.name = p_name;
    desc.format = p_format;
    desc.arraySize = p_array_size;
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
        case AttachmentType::COLOR_CUBE:
            desc.dimension = Dimension::TEXTURE_CUBE;
            desc.miscFlags |= RESOURCE_MISC_TEXTURECUBE;
            DEV_ASSERT(p_array_size == 6);
            break;
        case AttachmentType::SHADOW_CUBE_ARRAY:
            desc.dimension = Dimension::TEXTURE_CUBE_ARRAY;
            desc.miscFlags |= RESOURCE_MISC_TEXTURECUBE;
            DEV_ASSERT(p_array_size / 6 > 0);
            break;
        case AttachmentType::RW_TEXTURE:
            break;
        default:
            CRASH_NOW();
            break;
    }
    switch (p_type) {
        case AttachmentType::COLOR_2D:
        case AttachmentType::COLOR_CUBE:
            desc.bindFlags |= BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
            break;
        case AttachmentType::SHADOW_2D:
        case AttachmentType::SHADOW_CUBE_ARRAY:
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

}  // namespace my::renderer
