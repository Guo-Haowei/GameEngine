#include "pass_creator.h"

#include "core/debugger/profiler.h"
#include "core/framework/graphics_manager.h"
#include "core/math/geomath.h"
#include "rendering/graphics_dvars.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/render_manager.h"

// shader defines
#include "shader_resource_defines.hlsl.h"
#include "unordered_access_defines.hlsl.h"

// @TODO: refactor
// @TODO: this is temporary
#include "core/framework/scene_manager.h"
#include "drivers/opengl/opengl_graphics_manager.h"
#include "drivers/opengl/opengl_prerequisites.h"
#include "rendering/GpuTexture.h"
extern OldTexture g_albedoVoxel;
extern OldTexture g_normalVoxel;

namespace my::rg {

/// Gbuffer
static void GbufferPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();
    const uint32_t width = p_draw_pass->desc.depthAttachment->desc.width;
    const uint32_t height = p_draw_pass->desc.depthAttachment->desc.height;

    gm.SetRenderTarget(p_draw_pass);

    gm.SetViewport(Viewport(width, height));

    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT, clear_color);

    PassContext& pass = gm.m_mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    gm.SetPipelineState(PSO_GBUFFER);
    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), draw.bone_idx);
        }

        if (draw.flags) {
            gm.SetStencilRef(draw.flags);
        }

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);

        gm.SetMesh(draw.mesh_data);

        for (const auto& subset : draw.subsets) {
            const MaterialConstantBuffer& material = gm.GetCurrentFrame().materialCache.buffer[subset.material_idx];
            gm.BindTexture(Dimension::TEXTURE_2D, material.c_baseColorMapHandle, GetBaseColorMapSlot());
            gm.BindTexture(Dimension::TEXTURE_2D, material.c_normalMapHandle, GetNormalMapSlot());
            gm.BindTexture(Dimension::TEXTURE_2D, material.c_materialMapHandle, GetMaterialMapSlot());

            gm.BindConstantBufferSlot<MaterialConstantBuffer>(frame.materialCb.get(), subset.material_idx);

            // @TODO: set material

            gm.DrawElements(subset.index_count, subset.index_offset);

            // @TODO: unbind
        }

        if (draw.flags) {
            gm.SetStencilRef(0);
        }
    }
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

static void HighlightPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);
    const auto [width, height] = p_draw_pass->GetBufferSize();

    gm.SetViewport(Viewport(width, height));

    gm.SetPipelineState(PSO_HIGHLIGHT);
    gm.SetStencilRef(STENCIL_FLAG_SELECTED);
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);
    RenderManager::GetSingleton().draw_quad();
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
static void PointShadowPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    // prepare render data
    const auto [width, height] = p_draw_pass->GetBufferSize();

    for (int pass_id = 0; pass_id < MAX_POINT_LIGHT_SHADOW_COUNT; ++pass_id) {
        auto& pass_ptr = gm.m_pointShadowPasses[pass_id];
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
            for (const auto& draw : pass.draws) {
                const bool has_bone = draw.bone_idx >= 0;
                if (has_bone) {
                    gm.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), draw.bone_idx);
                }

                gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);

                gm.SetMesh(draw.mesh_data);
                gm.DrawElements(draw.mesh_data->indexCount);
            }
        }
    }
}

static void ShadowPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    gm.SetRenderTarget(p_draw_pass);
    const auto [width, height] = p_draw_pass->GetBufferSize();

    gm.Clear(p_draw_pass, CLEAR_DEPTH_BIT);

    gm.SetViewport(Viewport(width, height));

    PassContext& pass = gm.m_shadowPasses[0];
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    gm.SetPipelineState(PSO_DPETH);
    for (const auto& draw : pass.draws) {
        const bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), draw.bone_idx);
        }

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);

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

    auto shadow_map = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_SHADOW_MAP,
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

static void VoxelizationPassFunc(const DrawPass*) {
    OPTICK_EVENT();
    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    if (!DVAR_GET_BOOL(gfx_enable_vxgi)) {
        return;
    }

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

    g_albedoVoxel.clear();
    g_normalVoxel.clear();

    const int voxel_size = DVAR_GET_INT(gfx_voxel_size);

    glDisable(GL_BLEND);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glViewport(0, 0, voxel_size, voxel_size);

    // @TODO: fix
    g_albedoVoxel.bindImageTexture(IMAGE_VOXEL_ALBEDO_SLOT);
    g_normalVoxel.bindImageTexture(IMAGE_VOXEL_NORMAL_SLOT);

    PassContext& pass = gm.m_voxelPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    gm.SetPipelineState(PSO_VOXELIZATION);
    for (const auto& draw : pass.draws) {
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

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    // post process
    GraphicsManager::GetSingleton().SetPipelineState(PSO_VOXELIZATION_POST);

    constexpr GLuint workGroupX = 512;
    constexpr GLuint workGroupY = 512;
    const GLuint workGroupZ = (voxel_size * voxel_size * voxel_size) / (workGroupX * workGroupY);

    glDispatchCompute(workGroupX, workGroupY, workGroupZ);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    g_albedoVoxel.bind();
    g_albedoVoxel.genMipMap();
    g_normalVoxel.bind();
    g_normalVoxel.genMipMap();

    glEnable(GL_BLEND);

    // unbind stuff
    gm.UnbindTexture(Dimension::TEXTURE_2D, GetShadowMapSlot());
    gm.UnbindTexture(Dimension::TEXTURE_CUBE_ARRAY, GetPointShadowArraySlot());

    // @TODO: [SCRUM-28] refactor
    gm.UnsetRenderTarget();
}

void RenderPassCreator::AddVoxelizationPass() {
    GraphicsManager& manager = GraphicsManager::GetSingleton();

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
static void LightingPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    const auto [width, height] = p_draw_pass->GetBufferSize();

    gm.SetRenderTarget(p_draw_pass);

    gm.SetViewport(Viewport(width, height));
    gm.Clear(p_draw_pass, CLEAR_COLOR_BIT);
    gm.SetPipelineState(PSO_LIGHTING);

    RenderManager::GetSingleton().draw_quad();

    PassContext& pass = gm.m_mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.GetCurrentFrame().passCb.get(), pass.pass_idx);

    // if (0) {
    //     // draw billboard grass here for now
    //     manager.SetPipelineState(PSO_BILLBOARD);
    //     manager.setMesh(&g_grass);
    //     glDrawElementsInstanced(GL_TRIANGLES, g_grass.index_count, GL_UNSIGNED_INT, 0, 64);
    // }

    // @TODO: fix skybox
    // if (gm.GetBackend() == Backend::OPENGL) {
    //     GraphicsManager::GetSingleton().SetPipelineState(PSO_ENV_SKYBOX);
    //     RenderManager::GetSingleton().draw_skybox();
    // }
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
        .transitions = {
            ResourceTransition{
                .resource = manager.FindTexture(RESOURCE_GBUFFER_DEPTH),
                .slot = GetGbufferDepthSlot(),
                .beginPassFunc = [](GraphicsManager* p_graphics_manager,
                                    GpuTexture* p_texture,
                                    int p_slot) { p_graphics_manager->BindTexture(Dimension::TEXTURE_2D, p_texture->GetHandle(), p_slot); },
                .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                  GpuTexture*,
                                  int p_slot) { p_graphics_manager->UnbindTexture(Dimension::TEXTURE_2D, p_slot); },
            },
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
static void EmitterPassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    auto& gm = GraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();
    const auto [width, height] = p_draw_pass->GetBufferSize();

    gm.SetRenderTarget(p_draw_pass);
    gm.SetViewport(Viewport(width, height));

    PassContext& pass = gm.m_mainPass;
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
static void BloomSetupFunc(const DrawPass* p_draw_pass) {
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

static void BloomDownSampleFunc(const DrawPass* p_draw_pass) {
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

static void BloomUpSampleFunc(const DrawPass* p_draw_pass) {
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
                                                              PixelFormat::R11G11B10_FLOAT,
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
                                        int p_slot) { p_graphics_manager->SetUnorderedAccessView(p_slot, p_texture); },
                    .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                      GpuTexture*,
                                      int p_slot) { p_graphics_manager->SetUnorderedAccessView(p_slot, nullptr); },
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
                                        int p_slot) { p_graphics_manager->SetUnorderedAccessView(p_slot, p_texture); },
                    .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                      GpuTexture*,
                                      int p_slot) { p_graphics_manager->SetUnorderedAccessView(p_slot, nullptr); },
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
                                        int p_slot) { p_graphics_manager->SetUnorderedAccessView(p_slot, p_texture); },
                    .endPassFunc = [](GraphicsManager* p_graphics_manager,
                                      GpuTexture*,
                                      int p_slot) { p_graphics_manager->SetUnorderedAccessView(p_slot, nullptr); },
                } },
            .execFunc = BloomUpSampleFunc,
        });
        pass->id = i;
        render_pass->AddDrawPass(pass);
    }
}

/// Tone
/// Change to post processing?
static void TonePassFunc(const DrawPass* p_draw_pass) {
    OPTICK_EVENT();

    GraphicsManager& gm = GraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_draw_pass);

    auto depth_buffer = p_draw_pass->desc.depthAttachment;
    const auto [width, height] = p_draw_pass->GetBufferSize();

    // draw billboards

    // HACK:
    if (DVAR_GET_BOOL(gfx_debug_vxgi) && gm.GetBackend() == Backend::OPENGL) {
        // @TODO: remove
        extern void debug_vxgi_pass_func(const DrawPass* p_draw_pass);
        debug_vxgi_pass_func(p_draw_pass);
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
        RenderManager::GetSingleton().draw_quad();

        gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
        gm.UnbindTexture(Dimension::TEXTURE_2D, GetTextureLightingSlot());
        gm.UnbindTexture(Dimension::TEXTURE_2D, GetTextureHighlightSelectSlot());
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

    auto attachment = gm.CreateTexture(BuildDefaultTextureDesc(RESOURCE_TONE,
                                                               RESOURCE_FORMAT_TONE,
                                                               AttachmentType::COLOR_2D,
                                                               width, height),
                                       PointClampSampler());

    auto gbuffer_depth = gm.FindTexture(RESOURCE_GBUFFER_DEPTH);

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

    creator.AddShadowPass();
    creator.AddGbufferPass();
    creator.AddHighlightPass();
    creator.AddLightingPass();
    creator.AddBloomPass();
    creator.AddTonePass();

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
    creator.AddGbufferPass();
    creator.AddHighlightPass();
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

}  // namespace my::rg
