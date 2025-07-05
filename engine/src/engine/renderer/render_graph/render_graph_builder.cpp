#include "render_graph_builder.h"

#include "engine/assets/asset.h"
#include "engine/core/debugger/profiler.h"
#include "engine/math/matrix_transform.h"
#include "engine/renderer/base_graphics_manager.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/render_graph/render_graph_defines.h"
#include "engine/renderer/render_system.h"
#include "engine/renderer/renderer_misc.h"
#include "engine/renderer/sampler.h"
#include "engine/runtime/display_manager.h"

// @TODO: remove
#include "engine/runtime/asset_registry.h"

// @TODO: refactor
#define GBUFFER_PASS_NAME                "GbufferDrawPass"
#define VOXELIZATION_PASS_NAME           "VoxelizationDrawPass"
#define EARLY_Z_PASS_NAME                "EarlyZDrawPass"
#define SHADOW_DRAW_PASS_NAME            "ShadowDrawPass"
#define POINT_SHADOW_DRAW_PASS_NAME(IDX) "ShadowDrawPass" #IDX

namespace my {
#include "shader_resource_defines.hlsl.h"
#include "unordered_access_defines.hlsl.h"
}  // namespace my

namespace my::renderer {

struct FramebufferCreateInfo {
    std::vector<RenderTargetResourceName> colorAttachments;
    RenderTargetResourceName depthAttachment = RenderTargetResourceName::COUNT;
};

struct DrawPassCreateInfo {
    FramebufferCreateInfo framebuffer;
    DrawPassExecuteFunc func{ nullptr };
};

struct RenderPassCreateInfo {
    RenderPassName name;
    std::vector<RenderPassName> dependencies;
    std::vector<DrawPassCreateInfo> drawPasses;
};

/// Gbuffer

// @TODO: generalize this
static void DrawInstacedGeometry(const RenderData& p_data, const std::vector<InstanceContext>& p_instances, bool p_is_prepass) {
    unused(p_is_prepass);

    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    for (const auto& instance : p_instances) {
        DEV_ASSERT(instance.instanceBufferIndex >= 0);
        gm.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), instance.instanceBufferIndex);

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), instance.batchIdx);

        gm.SetMesh(instance.gpuMesh);

        const MaterialConstantBuffer& material = p_data.materialCache.buffer[instance.materialIdx];
        gm.BindTexture(Dimension::TEXTURE_2D, material.c_baseColorMapHandle, GetBaseColorMapSlot());
        gm.BindTexture(Dimension::TEXTURE_2D, material.c_normalMapHandle, GetNormalMapSlot());
        gm.BindTexture(Dimension::TEXTURE_2D, material.c_materialMapHandle, GetMaterialMapSlot());

        gm.BindConstantBufferSlot<MaterialConstantBuffer>(frame.materialCb.get(), instance.materialIdx);

        gm.DrawElementsInstanced(instance.instanceCount,
                                 instance.indexCount,
                                 instance.indexOffset);
    }
}

static void ExecuteDrawCommands(const RenderData& p_data, const DrawPass& p_draw_pass, bool p_is_prepass = false) {

    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    for (const RenderCommand& cmd : p_draw_pass.commands) {
        if (cmd.type != RenderCommandType::Draw) continue;
        const DrawCommand& draw = cmd.draw;

        const bool has_bone = draw.bone_idx >= 0;
        if (has_bone) {
            gm.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), draw.bone_idx);
        }

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);

        gm.SetMesh(draw.mesh_data);

        // @TODO: instead of dowing this,
        // set flag directly from draw.flags
        if (p_is_prepass && draw.flags) {
            gm.SetStencilRef(draw.flags);
        }

        if (draw.mat_idx != -1) {
            const MaterialConstantBuffer& material = p_data.materialCache.buffer[draw.mat_idx];
            gm.BindTexture(Dimension::TEXTURE_2D, material.c_baseColorMapHandle, GetBaseColorMapSlot());
            gm.BindTexture(Dimension::TEXTURE_2D, material.c_normalMapHandle, GetNormalMapSlot());
            gm.BindTexture(Dimension::TEXTURE_2D, material.c_materialMapHandle, GetMaterialMapSlot());

            gm.BindConstantBufferSlot<MaterialConstantBuffer>(frame.materialCb.get(), draw.mat_idx);
        }
        gm.DrawElements(draw.indexCount, draw.indexOffset);

        if (p_is_prepass && draw.flags) {
            gm.SetStencilRef(0);
        }
    }
}

static void PrepassFunc(const RenderData& p_data, const Framebuffer*, DrawPass& p_pass,
                        IRenderCmdContext&) {
    HBN_PROFILE_EVENT();

    Framebuffer* p_framebuffer = p_pass.framebuffer.get();
    auto& gm = IGraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();
    const uint32_t width = p_framebuffer->desc.depthAttachment->desc.width;
    const uint32_t height = p_framebuffer->desc.depthAttachment->desc.height;

    gm.SetRenderTarget(p_framebuffer);
    gm.SetViewport(Viewport(width, height));

    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    gm.Clear(p_framebuffer, CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT, clear_color, 0.0f, STENCIL_FLAG_SKY);

    const PassContext& pass = p_data.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    gm.SetPipelineState(PSO_PREPASS);
    ExecuteDrawCommands(p_data, p_pass, true);
}

static void ForwardPass(const RenderData& p_data, const Framebuffer* p_framebuffer, DrawPass&,
                        IRenderCmdContext&) {
    unused(p_data);
    HBN_PROFILE_EVENT();
    auto& gm = IGraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_framebuffer);
    float clear_color[] = { 0.3f, 0.5f, 0.4f, 1.0f };
    gm.Clear(p_framebuffer, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT, clear_color);
}

void RenderGraphBuilder::AddForward() {
    RenderPassCreateInfo info{
        .name = RenderPassName::FINAL,
        .drawPasses = {
            { {
                  { RESOURCE_FINAL },
                  RESOURCE_GBUFFER_DEPTH,
              },
              ForwardPass },
        }
    };

    CreateRenderPass(info);
}

void RenderGraphBuilder::AddPrepass() {
    auto& manager = IGraphicsManager::GetSingleton();

    auto depth = manager.FindTexture(RESOURCE_GBUFFER_DEPTH);

    RenderPassDesc desc;
    desc.name = RenderPassName::PREPASS;
    auto pass = m_graph.CreatePass(desc);

    FramebufferDesc fb_desc;
    fb_desc.depthAttachment = depth;

    auto framebuffer = manager.CreateFramebuffer(fb_desc);
    pass->AddDrawPass(EARLY_Z_PASS_NAME, framebuffer, PrepassFunc);
}

static void GbufferPassFunc(const RenderData& p_data, const Framebuffer*, DrawPass& p_pass,
                            IRenderCmdContext& p_cmd) {
    HBN_PROFILE_EVENT();

    const Framebuffer* framebuffer = p_pass.framebuffer.get();

    const auto& frame = p_cmd.GetCurrentFrame();
    const uint32_t width = framebuffer->desc.depthAttachment->desc.width;
    const uint32_t height = framebuffer->desc.depthAttachment->desc.height;

    p_cmd.SetRenderTarget(framebuffer);
    p_cmd.SetViewport(Viewport(width, height));

    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    p_cmd.Clear(framebuffer, CLEAR_COLOR_BIT, clear_color);

    const PassContext& pass = p_data.mainPass;
    p_cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    p_cmd.SetPipelineState(PSO_GBUFFER);
    ExecuteDrawCommands(p_data, p_pass, false);
    DrawInstacedGeometry(p_data, p_data.instances, false);
    p_cmd.SetPipelineState(PSO_GBUFFER_DOUBLE_SIDED);
}

void RenderGraphBuilder::AddGbufferPass() {
    auto& manager = IGraphicsManager::GetSingleton();

    auto color0 = manager.FindTexture(RESOURCE_GBUFFER_BASE_COLOR);
    auto color1 = manager.FindTexture(RESOURCE_GBUFFER_NORMAL);
    auto color2 = manager.FindTexture(RESOURCE_GBUFFER_MATERIAL);
    auto depth = manager.FindTexture(RESOURCE_GBUFFER_DEPTH);

    RenderPassDesc desc;
    desc.name = RenderPassName::GBUFFER;
    auto pass = m_graph.CreatePass(desc);

    FramebufferDesc fb_desc;
    fb_desc.colorAttachments.emplace_back(color0);
    fb_desc.colorAttachments.emplace_back(color1);
    fb_desc.colorAttachments.emplace_back(color2);
    fb_desc.depthAttachment = depth;

    auto framebuffer = manager.CreateFramebuffer(fb_desc);
    pass->AddDrawPass(GBUFFER_PASS_NAME, framebuffer, GbufferPassFunc);
}

static void SsaoPassFunc(const RenderData& p_data, const Framebuffer* p_framebuffer, DrawPass&,
                         IRenderCmdContext&) {
    if (!p_data.options.ssaoEnabled) {
        return;
    }

    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();
    const uint32_t width = p_framebuffer->desc.colorAttachments[0]->desc.width;
    const uint32_t height = p_framebuffer->desc.colorAttachments[0]->desc.height;

    gm.SetRenderTarget(p_framebuffer);
    gm.SetViewport(Viewport(width, height));
    gm.Clear(p_framebuffer, CLEAR_COLOR_BIT);

    const PassContext& pass = p_data.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    auto noise = renderer::GetSsaoNoiseTexture();
    gm.BindTexture(Dimension::TEXTURE_2D, noise->GetHandle(), 0);
    gm.SetPipelineState(PSO_SSAO);
    gm.DrawQuad();
}

void RenderGraphBuilder::AddSsaoPass() {
    RenderPassCreateInfo info{
        .name = RenderPassName::SSAO,
        .dependencies = { RenderPassName::GBUFFER },
        .drawPasses = {
            { { { RESOURCE_SSAO } },
              SsaoPassFunc },
        },
    };

    CreateRenderPass(info);
}

static void HighlightPassFunc(const RenderData&, const Framebuffer* p_framebuffer, DrawPass&,
                              IRenderCmdContext&) {
    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_framebuffer);
    const auto [width, height] = p_framebuffer->GetBufferSize();

    gm.SetViewport(Viewport(width, height));

    gm.SetPipelineState(PSO_HIGHLIGHT);
    gm.SetStencilRef(STENCIL_FLAG_SELECTED);
    gm.Clear(p_framebuffer, CLEAR_COLOR_BIT);
    gm.DrawQuad();
    gm.SetStencilRef(0);
}

void RenderGraphBuilder::AddHighlightPass() {
    RenderPassCreateInfo info{
        .name = RenderPassName::OUTLINE,
        .dependencies = { RenderPassName::PREPASS },
        .drawPasses = {
            { { { RESOURCE_OUTLINE_SELECT },
                RESOURCE_GBUFFER_DEPTH },
              HighlightPassFunc },
        },
    };

    CreateRenderPass(info);
}

/// Shadow
// @TODO: make multiple
static void PointShadowPassFunc(const RenderData& p_data, const Framebuffer*, DrawPass& p_pass,
                                IRenderCmdContext& p_cmd) {
    HBN_PROFILE_EVENT();

    auto& gm = p_cmd;
    const Framebuffer* framebuffer = p_pass.framebuffer.get();
    auto& frame = gm.GetCurrentFrame();

    // prepare render data
    const auto [width, height] = framebuffer->GetBufferSize();

    for (int pass_id = 0; pass_id < MAX_POINT_LIGHT_SHADOW_COUNT; ++pass_id) {
        auto& pass_ptr = p_data.pointShadowPasses[pass_id];
        if (!pass_ptr) {
            continue;
        }

        for (int face_id = 0; face_id < 6; ++face_id) {
            const uint32_t slot = pass_id * 6 + face_id;
            gm.BindConstantBufferSlot<PointShadowConstantBuffer>(frame.pointShadowCb.get(), slot);

            gm.SetRenderTarget(framebuffer, slot);
            gm.Clear(framebuffer, CLEAR_DEPTH_BIT, nullptr, 1.0f, 0, slot);

            gm.SetViewport(Viewport(width, height));

            gm.SetPipelineState(PSO_POINT_SHADOW);
            ExecuteDrawCommands(p_data, p_pass, false);
        }
    }
}

static void ShadowPassFunc(const RenderData& p_data, const Framebuffer*, DrawPass& p_pass,
                           IRenderCmdContext& p_cmd) {
    HBN_PROFILE_EVENT();

    const Framebuffer* framebuffer = p_pass.framebuffer.get();
    auto& gm = p_cmd;
    const auto& frame = gm.GetCurrentFrame();

    gm.SetRenderTarget(framebuffer);
    const auto [width, height] = framebuffer->GetBufferSize();

    gm.Clear(framebuffer, CLEAR_DEPTH_BIT);

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
    ExecuteDrawCommands(p_data, p_pass);
}

void RenderGraphBuilder::AddShadowPass() {
    IGraphicsManager& manager = IGraphicsManager::GetSingleton();

    const int shadow_res = DVAR_GET_INT(gfx_shadow_res);
    DEV_ASSERT(IsPowerOfTwo(shadow_res));
    const int point_shadow_res = DVAR_GET_INT(gfx_point_shadow_res);
    DEV_ASSERT(IsPowerOfTwo(point_shadow_res));

    auto shadow_map = manager.CreateTexture(BuildDefaultTextureDesc(RESOURCE_SHADOW_MAP,
                                                                    PixelFormat::D32_FLOAT,
                                                                    AttachmentType::SHADOW_2D,
                                                                    1 * shadow_res, shadow_res),
                                            ShadowMapSampler());
    RenderPassDesc desc;
    desc.name = RenderPassName::SHADOW;
    auto pass = m_graph.CreatePass(desc);
    {
        auto framebuffer = manager.CreateFramebuffer(FramebufferDesc{ .depthAttachment = shadow_map });
        pass->AddDrawPass(SHADOW_DRAW_PASS_NAME, framebuffer, ShadowPassFunc);
    }

    auto point_shadowMap = manager.CreateTexture(BuildDefaultTextureDesc(static_cast<RenderTargetResourceName>(RESOURCE_POINT_SHADOW_CUBE_ARRAY),
                                                                         PixelFormat::D32_FLOAT,
                                                                         AttachmentType::SHADOW_CUBE_ARRAY,
                                                                         point_shadow_res, point_shadow_res, 6 * MAX_POINT_LIGHT_SHADOW_COUNT),
                                                 ShadowMapSampler());

    auto framebuffer = manager.CreateFramebuffer(FramebufferDesc{ .depthAttachment = point_shadowMap });
    pass->AddDrawPass(POINT_SHADOW_DRAW_PASS_NAME(0), framebuffer, PointShadowPassFunc);
}

static void VoxelizationPassFunc(const RenderData& p_data, const Framebuffer*, DrawPass& p_pass,
                                 IRenderCmdContext& p_cmd) {
    if (p_data.voxelPass.pass_idx < 0) {
        return;
    }

    HBN_PROFILE_EVENT();
    auto& gm = p_cmd;
    const auto& frame = gm.GetCurrentFrame();

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
        ExecuteDrawCommands(p_data, p_pass);

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

void RenderGraphBuilder::AddVoxelizationPass() {
    auto& manager = IGraphicsManager::GetSingleton();
    if (manager.GetBackend() != Backend::OPENGL) {
        return;
    }

    {
        const int voxel_size = DVAR_GET_INT(gfx_voxel_size);
        GpuTextureDesc desc = BuildDefaultTextureDesc(RESOURCE_VOXEL_LIGHTING,
                                                      PixelFormat::R16G16B16A16_FLOAT,
                                                      AttachmentType::RW_TEXTURE,
                                                      voxel_size, voxel_size);
        desc.dimension = Dimension::TEXTURE_3D;
        desc.mipLevels = LogTwo(voxel_size);
        desc.depth = voxel_size;
        desc.miscFlags |= RESOURCE_MISC_GENERATE_MIPS;
        desc.bindFlags |= BIND_RENDER_TARGET | BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;

        SamplerDesc sampler(MinFilter::LINEAR_MIPMAP_LINEAR, MagFilter::POINT, AddressMode::BORDER);

        auto voxel_lighting = manager.CreateTexture(desc, sampler);

        desc.name = RESOURCE_VOXEL_NORMAL;
        auto voxel_normal = manager.CreateTexture(desc, sampler);
    }

    RenderPassDesc desc;
    desc.name = RenderPassName::VOXELIZATION;
    desc.dependencies = { RenderPassName::SHADOW };
    auto pass = m_graph.CreatePass(desc);
    auto framebuffer = manager.CreateFramebuffer(FramebufferDesc{});
    pass->AddDrawPass(VOXELIZATION_PASS_NAME, framebuffer, VoxelizationPassFunc);
}

/// Emitter
static void EmitterPassFunc(const RenderData& p_data, const Framebuffer* p_framebuffer) {
    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();
    const auto [width, height] = p_framebuffer->GetBufferSize();

    gm.SetRenderTarget(p_framebuffer);
    gm.SetViewport(Viewport(width, height));

    const PassContext& pass = p_data.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    int particle_idx = 0;
    for (const auto& emitter : p_data.emitters) {
        if (!emitter.particleBuffer) {
            continue;
        }

        gm.BindConstantBufferSlot<EmitterConstantBuffer>(frame.emitterCb.get(), particle_idx);
        ++particle_idx;

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

        bool use_texture = false;
        if (!emitter.texture.empty()) {
            const ImageAsset* image = AssetRegistry::GetSingleton().GetAssetByHandle<ImageAsset>(emitter.texture);
            if (image && image->gpu_texture) {
                gm.BindTexture(Dimension::TEXTURE_2D, image->gpu_texture->GetHandle(), GetBaseColorMapSlot());
                use_texture = true;
            }
        }

        gm.BindStructuredBufferSRV(GetGlobalParticleDataSlot(), emitter.particleBuffer.get());
        gm.DrawQuadInstanced(MAX_PARTICLE_COUNT);
        gm.UnbindStructuredBufferSRV(GetGlobalParticleDataSlot());

        if (use_texture) {
            gm.UnbindTexture(Dimension::TEXTURE_2D, GetBaseColorMapSlot());
        }
    }
}

/// Lighting
static void LightingPassFunc(const RenderData&, const Framebuffer* p_framebuffer, DrawPass&,
                             IRenderCmdContext&) {
    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    const auto [width, height] = p_framebuffer->GetBufferSize();

    gm.SetRenderTarget(p_framebuffer);

    gm.SetViewport(Viewport(width, height));
    gm.Clear(p_framebuffer, CLEAR_COLOR_BIT);
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
}

void RenderGraphBuilder::AddLightingPass() {
    auto& manager = IGraphicsManager::GetSingleton();

    auto lighting_attachment = manager.FindTexture(RESOURCE_LIGHTING);

    RenderPassDesc desc;
    desc.name = RenderPassName::LIGHTING;

    desc.dependencies = { RenderPassName::GBUFFER,
                          RenderPassName::ENV,
                          RenderPassName::SSAO };

    if (m_config.enableShadow) {
        desc.dependencies.push_back(RenderPassName::SHADOW);
    }
    if (m_config.enableVxgi) {
        desc.dependencies.push_back(RenderPassName::VOXELIZATION);
    }

    auto pass = m_graph.CreatePass(desc);
    auto framebuffer = manager.CreateFramebuffer(FramebufferDesc{
        .colorAttachments = { lighting_attachment },
        // @TODO: refactor transitions
        .transitions = {
            ResourceTransition{
                .resource = manager.FindTexture(RESOURCE_SHADOW_MAP),
                .slot = GetShadowMapSlot(),
                .beginPassFunc = [](IGraphicsManager* p_graphics_manager,
                                    GpuTexture* p_texture,
                                    int p_slot) { p_graphics_manager->BindTexture(Dimension::TEXTURE_2D, p_texture->GetHandle(), p_slot); },
                .endPassFunc = [](IGraphicsManager* p_graphics_manager,
                                  GpuTexture*,
                                  int p_slot) { p_graphics_manager->UnbindTexture(Dimension::TEXTURE_2D, p_slot); },
            },
            ResourceTransition{
                .resource = manager.FindTexture(RESOURCE_POINT_SHADOW_CUBE_ARRAY),
                .slot = GetPointShadowArraySlot(),
                .beginPassFunc = [](IGraphicsManager* p_graphics_manager,
                                    GpuTexture* p_texture,
                                    int p_slot) { p_graphics_manager->BindTexture(Dimension::TEXTURE_CUBE_ARRAY, p_texture->GetHandle(), p_slot); },
                .endPassFunc = [](IGraphicsManager* p_graphics_manager,
                                  GpuTexture*,
                                  int p_slot) { p_graphics_manager->UnbindTexture(Dimension::TEXTURE_CUBE_ARRAY, p_slot); },
            },
        },
    });
    pass->AddDrawPass("LightingDrawPass", framebuffer, LightingPassFunc);
}

/// Sky
static void SkyPassFunc(const RenderData& p_data, const Framebuffer*, DrawPass& p_pass,
                        IRenderCmdContext& p_cmd) {
    HBN_PROFILE_EVENT();

    auto& gm = p_cmd;
    const auto [width, height] = p_pass.framebuffer->GetBufferSize();

    gm.SetRenderTarget(p_pass.framebuffer.get());

    gm.SetViewport(Viewport(width, height));

    const PassContext& pass = p_data.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.GetCurrentFrame().passCb.get(), pass.pass_idx);

    {
#if 1
        auto skybox = gm.FindTexture(RESOURCE_ENV_PREFILTER_CUBE_MAP);
        constexpr int skybox_slot = GetPrefilteredSlot();
#else
        auto skybox = gm.FindTexture(RESOURCE_ENV_SKYBOX_CUBE_MAP);
        constexpr int skybox_slot = GetSkyboxSlot();
#endif
        if (skybox) {
            gm.BindTexture(Dimension::TEXTURE_CUBE, skybox->GetHandle(), skybox_slot);
            gm.SetPipelineState(PSO_ENV_SKYBOX);
            gm.SetStencilRef(STENCIL_FLAG_SKY);
            gm.DrawSkybox();
            gm.SetStencilRef(0);
            gm.UnbindTexture(Dimension::TEXTURE_CUBE, skybox_slot);
        }
    }

    // draw transparent objects
    gm.SetPipelineState(PSO_FORWARD_TRANSPARENT);

    // @TODO: this is hacky...
    ExecuteDrawCommands(p_data, p_pass);

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

    EmitterPassFunc(p_data, p_pass.framebuffer.get());
}

void RenderGraphBuilder::AddSkyPass() {
    auto& manager = IGraphicsManager::GetSingleton();

    auto gbuffer_depth = manager.FindTexture(RESOURCE_GBUFFER_DEPTH);

    auto lighting_attachment = manager.FindTexture(RESOURCE_LIGHTING);

    RenderPassDesc desc;
    desc.name = RenderPassName::SKY;

    desc.dependencies = { RenderPassName::LIGHTING };

    auto pass = m_graph.CreatePass(desc);
    auto framebuffer = manager.CreateFramebuffer(FramebufferDesc{
        .colorAttachments = { lighting_attachment },
        .depthAttachment = gbuffer_depth,
        // @TODO: refactor transitions
        .transitions = {
            ResourceTransition{
                .resource = manager.FindTexture(RESOURCE_SHADOW_MAP),
                .slot = GetShadowMapSlot(),
                .beginPassFunc = [](IGraphicsManager* p_graphics_manager,
                                    GpuTexture* p_texture,
                                    int p_slot) { p_graphics_manager->BindTexture(Dimension::TEXTURE_2D, p_texture->GetHandle(), p_slot); },
                .endPassFunc = [](IGraphicsManager* p_graphics_manager,
                                  GpuTexture*,
                                  int p_slot) { p_graphics_manager->UnbindTexture(Dimension::TEXTURE_2D, p_slot); },
            },
            ResourceTransition{
                .resource = manager.FindTexture(RESOURCE_POINT_SHADOW_CUBE_ARRAY),
                .slot = GetPointShadowArraySlot(),
                .beginPassFunc = [](IGraphicsManager* p_graphics_manager,
                                    GpuTexture* p_texture,
                                    int p_slot) { p_graphics_manager->BindTexture(Dimension::TEXTURE_CUBE_ARRAY, p_texture->GetHandle(), p_slot); },
                .endPassFunc = [](IGraphicsManager* p_graphics_manager,
                                  GpuTexture*,
                                  int p_slot) { p_graphics_manager->UnbindTexture(Dimension::TEXTURE_CUBE_ARRAY, p_slot); },
            },
        },
    });

    // @TODO: refactor
    pass->AddDrawPass("SkyDrawPass", framebuffer, SkyPassFunc);
}

/// Bloom
static void BloomSetupFunc(const RenderData& p_data, const Framebuffer*, DrawPass&,
                           IRenderCmdContext&) {
    if (!p_data.options.bloomEnabled) {
        return;
    }

    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    gm.SetPipelineState(PSO_BLOOM_SETUP);
    auto input = gm.FindTexture(RESOURCE_LIGHTING);

    const uint32_t width = input->desc.width;
    const uint32_t height = input->desc.height;
    const uint32_t work_group_x = CeilingDivision(width, 16);
    const uint32_t work_group_y = CeilingDivision(height, 16);

    gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), 0);

    gm.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), GetBloomInputTextureSlot());
    gm.Dispatch(work_group_x, work_group_y, 1);
    gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

static void BloomDownSampleFunc(const RenderData& p_data, const Framebuffer* p_framebuffer, DrawPass&,
                                IRenderCmdContext&) {
    if (!p_data.options.bloomEnabled) {
        return;
    }

    HBN_PROFILE_EVENT();

    const uint32_t pass_id = p_framebuffer->id;
    auto& gm = IGraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();

    gm.SetPipelineState(PSO_BLOOM_DOWNSAMPLE);
    auto input = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + pass_id));

    DEV_ASSERT(input);

    const uint32_t width = input->desc.width;
    const uint32_t height = input->desc.height;
    const uint32_t work_group_x = CeilingDivision(width, 16);
    const uint32_t work_group_y = CeilingDivision(height, 16);

    gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), pass_id + 1);

    gm.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), GetBloomInputTextureSlot());
    gm.Dispatch(work_group_x, work_group_y, 1);
    gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

static void BloomUpSampleFunc(const RenderData& p_data, const Framebuffer* p_framebuffer, DrawPass&,
                              IRenderCmdContext&) {
    if (!p_data.options.bloomEnabled) {
        return;
    }

    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    const uint32_t pass_id = p_framebuffer->id;
    auto& frame = gm.GetCurrentFrame();

    gm.SetPipelineState(PSO_BLOOM_UPSAMPLE);
    auto input = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + pass_id));
    auto output = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + pass_id - 1));

    const uint32_t width = output->desc.width;
    const uint32_t height = output->desc.height;
    const uint32_t work_group_x = CeilingDivision(width, 16);
    const uint32_t work_group_y = CeilingDivision(height, 16);

    gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), pass_id + BLOOM_MIP_CHAIN_MAX);

    gm.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), GetBloomInputTextureSlot());
    gm.Dispatch(work_group_x, work_group_y, 1);
    gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

void RenderGraphBuilder::AddBloomPass() {
    auto& gm = IGraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::BLOOM;
    desc.dependencies = { RenderPassName::SKY };
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
        auto pass = gm.CreateFramebuffer(FramebufferDesc{
            .transitions = {
                ResourceTransition{
                    .resource = output,
                    .slot = GetUavSlotBloomOutputImage(),
                    .beginPassFunc = [](IGraphicsManager* p_graphics_manager,
                                        GpuTexture* p_texture,
                                        int p_slot) { p_graphics_manager->BindUnorderedAccessView(p_slot, p_texture); },
                    .endPassFunc = [](IGraphicsManager* p_graphics_manager,
                                      GpuTexture*,
                                      int p_slot) { p_graphics_manager->UnbindUnorderedAccessView(p_slot); },
                } },
        });
        render_pass->AddDrawPass("BloomSetupComputePass", pass, BloomSetupFunc);
    }

    // Down Sample
    for (int i = 0; i < BLOOM_MIP_CHAIN_MAX - 1; ++i) {
        auto output = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i + 1));
        DEV_ASSERT(output);
        auto pass = gm.CreateFramebuffer(FramebufferDesc{
            .transitions = {
                ResourceTransition{
                    .resource = output,
                    .slot = GetUavSlotBloomOutputImage(),
                    .beginPassFunc = [](IGraphicsManager* p_graphics_manager,
                                        GpuTexture* p_texture,
                                        int p_slot) { p_graphics_manager->BindUnorderedAccessView(p_slot, p_texture); },
                    .endPassFunc = [](IGraphicsManager* p_graphics_manager,
                                      GpuTexture*,
                                      int p_slot) { p_graphics_manager->UnbindUnorderedAccessView(p_slot); },
                } },
        });
        pass->id = i;
        render_pass->AddDrawPass("BloomDownSampleComputePass", pass, BloomDownSampleFunc);
    }

    // Up Sample
    for (int i = BLOOM_MIP_CHAIN_MAX - 1; i > 0; --i) {
        auto output = gm.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + i - 1));
        DEV_ASSERT(output);
        auto pass = gm.CreateFramebuffer(FramebufferDesc{
            .transitions = {
                ResourceTransition{
                    .resource = output,
                    .slot = GetUavSlotBloomOutputImage(),
                    .beginPassFunc = [](IGraphicsManager* p_graphics_manager,
                                        GpuTexture* p_texture,
                                        int p_slot) { p_graphics_manager->BindUnorderedAccessView(p_slot, p_texture); },
                    .endPassFunc = [](IGraphicsManager* p_graphics_manager,
                                      GpuTexture*,
                                      int p_slot) { p_graphics_manager->UnbindUnorderedAccessView(p_slot); },
                } },
        });
        pass->id = i;
        render_pass->AddDrawPass("BloomUpSampleComputePass", pass, BloomUpSampleFunc);
    }
}

static void DebugVoxels(const RenderData& p_data, const Framebuffer* p_framebuffer) {
    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_framebuffer);
    auto depth_buffer = p_framebuffer->desc.depthAttachment;
    const auto [width, height] = p_framebuffer->GetBufferSize();

    // glEnable(GL_BLEND);
    gm.SetViewport(Viewport(width, height));
    gm.Clear(p_framebuffer, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT, IGraphicsManager::DEFAULT_CLEAR_COLOR, 0.0f);

    IGraphicsManager::GetSingleton().SetPipelineState(PSO_DEBUG_VOXEL);

    const PassContext& pass = p_data.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.GetCurrentFrame().passCb.get(), pass.pass_idx);

    gm.SetMesh(gm.m_boxBuffers.get());
    const uint32_t size = DVAR_GET_INT(gfx_voxel_size);
    gm.DrawElementsInstanced(size * size * size, gm.m_boxBuffers->desc.drawCount);

    // glDisable(GL_BLEND);
}

/// Tone
/// Change to post processing?
static void TonePassFunc(const RenderData& p_data, const Framebuffer* p_framebuffer, DrawPass&,
                         IRenderCmdContext&) {
    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_framebuffer);

    auto depth_buffer = p_framebuffer->desc.depthAttachment;
    const auto [width, height] = p_framebuffer->GetBufferSize();

    // draw billboards

    // @HACK:
    if (DVAR_GET_BOOL(gfx_debug_vxgi) && gm.GetBackend() == Backend::OPENGL) {
        // @TODO: remove
        DebugVoxels(p_data, p_framebuffer);
    } else {
        auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
            std::shared_ptr<GpuTexture> resource = gm.FindTexture(p_name);
            if (!resource) {
                return;
            }

            gm.BindTexture(p_dimension, resource->GetHandle(), p_slot);
        };
        bind_slot(RESOURCE_LIGHTING, GetTextureLightingSlot());
        bind_slot(RESOURCE_OUTLINE_SELECT, GetTextureHighlightSelectSlot());
        bind_slot(RESOURCE_BLOOM_0, GetBloomInputTextureSlot());

        gm.SetViewport(Viewport(width, height));
        gm.Clear(p_framebuffer, CLEAR_COLOR_BIT);

        gm.SetPipelineState(PSO_TONE);
        gm.DrawQuad();

        gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
        gm.UnbindTexture(Dimension::TEXTURE_2D, GetTextureLightingSlot());
        gm.UnbindTexture(Dimension::TEXTURE_2D, GetTextureHighlightSelectSlot());
    }
}

void RenderGraphBuilder::CreateRenderPass(RenderPassCreateInfo& p_info) {
    auto& gm = IGraphicsManager::GetSingleton();

    auto pass = m_graph.CreatePass(reinterpret_cast<RenderPassDesc&>(p_info));
    for (const auto& info : p_info.drawPasses) {
        FramebufferDesc framebuffer_desc;
        for (const auto color : info.framebuffer.colorAttachments) {
            auto resource = gm.FindTexture(color);
            DEV_ASSERT(resource);
            framebuffer_desc.colorAttachments.emplace_back(resource);
        }
        if (info.framebuffer.depthAttachment != RenderTargetResourceName::COUNT) {
            auto resource = gm.FindTexture(info.framebuffer.depthAttachment);
            DEV_ASSERT(resource);
            framebuffer_desc.depthAttachment = resource;
        }

        auto framebuffer = gm.CreateFramebuffer(framebuffer_desc);
        pass->AddDrawPass("xxxx", framebuffer, info.func);
    }
}

void RenderGraphBuilder::AddTonePass() {
    RenderPassCreateInfo info{
        .name = RenderPassName::TONE,
        .dependencies = { RenderPassName::BLOOM,
                          RenderPassName::OUTLINE },
        .drawPasses = {
            { { { RESOURCE_TONE }, RESOURCE_GBUFFER_DEPTH }, TonePassFunc },
        },
    };

    CreateRenderPass(info);
}

// assume render target is setup
void RenderGraphBuilder::DrawDebugImages(const RenderData& p_data, int p_width, int p_height, IGraphicsManager& p_graphics_manager) {
    auto& frame = p_graphics_manager.GetCurrentFrame();

    p_graphics_manager.SetViewport(Viewport(p_width, p_height));
    p_graphics_manager.SetPipelineState(PSO_RW_TEXTURE_2D);

    uint32_t offset = p_data.drawImageOffset;
    for (const auto& draw_context : p_data.drawImageContext) {
        p_graphics_manager.BindTexture(Dimension::TEXTURE_2D, draw_context.handle, GetBaseColorMapSlot());
        p_graphics_manager.BindConstantBufferSlot<MaterialConstantBuffer>(frame.materialCb.get(), offset++);
        p_graphics_manager.DrawQuad();
        p_graphics_manager.UnbindTexture(Dimension::TEXTURE_2D, GetBaseColorMapSlot());
    }
}

void RenderGraphBuilder::AddDebugImagePass() {
    if (m_config.is_runtime) {
        return;
    }

    RenderPassCreateInfo info{
        .name = RenderPassName::FINAL,
        .dependencies = { RenderPassName::TONE },
        .drawPasses = {
            { { { RESOURCE_FINAL } },
              [](const RenderData& p_data, const Framebuffer* p_framebuffer, DrawPass&,
                 IRenderCmdContext&) {
                  HBN_PROFILE_EVENT();
                  auto& gm = IGraphicsManager::GetSingleton();
                  gm.SetRenderTarget(p_framebuffer);
                  gm.Clear(p_framebuffer, CLEAR_COLOR_BIT);

                  const int width = p_framebuffer->desc.colorAttachments[0]->desc.width;
                  const int height = p_framebuffer->desc.colorAttachments[0]->desc.height;
                  DrawDebugImages(p_data, width, height, gm);
              } },
        },
    };

    CreateRenderPass(info);
}

static void ConvertToCubemapFunc(const RenderData& p_data, const Framebuffer* p_framebuffer, DrawPass&,
                                 IRenderCmdContext&) {
    HBN_PROFILE_EVENT("hdr image to -> skybox");
    if (!p_data.bakeIbl) {
        return;
    }

    auto& gm = IGraphicsManager::GetSingleton();

    gm.SetPipelineState(PSO_ENV_SKYBOX_TO_CUBE_MAP);
    auto cube_map = p_framebuffer->desc.colorAttachments[0];
    const auto [width, height] = p_framebuffer->GetBufferSize();

    auto& frame = gm.GetCurrentFrame();
    gm.BindTexture(Dimension::TEXTURE_2D, p_data.skyboxHdr->GetHandle(), GetSkyboxHdrSlot());
    for (int i = 0; i < 6; ++i) {
        gm.SetRenderTarget(p_framebuffer, i);

        gm.SetViewport(Viewport(width, height));

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
        gm.DrawSkybox();
    }
    gm.UnbindTexture(Dimension::TEXTURE_2D, GetSkyboxHdrSlot());
    gm.GenerateMipmap(cube_map.get());
}

static void DiffuseIrradianceFunc(const RenderData& p_data, const Framebuffer* p_framebuffer, DrawPass&,
                                  IRenderCmdContext&) {
    if (!p_data.bakeIbl) {
        return;
    }
    HBN_PROFILE_EVENT("bake diffuse irradiance");

    auto& gm = IGraphicsManager::GetSingleton();

    gm.SetPipelineState(PSO_DIFFUSE_IRRADIANCE);
    const auto [width, height] = p_framebuffer->GetBufferSize();

    auto skybox = gm.FindTexture(RESOURCE_ENV_SKYBOX_CUBE_MAP);
    DEV_ASSERT(skybox);
    gm.BindTexture(Dimension::TEXTURE_CUBE, skybox->GetHandle(), GetSkyboxSlot());
    auto& frame = gm.GetCurrentFrame();
    for (int i = 0; i < 6; ++i) {
        gm.SetRenderTarget(p_framebuffer, i);
        gm.SetViewport(Viewport(width, height));

        gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
        gm.DrawSkybox();
    }
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, GetSkyboxSlot());
}

static void PrefilteredFunc(const RenderData& p_data, const Framebuffer* p_framebuffer, DrawPass&,
                            IRenderCmdContext&) {
    if (!p_data.bakeIbl) {
        return;
    }
    HBN_PROFILE_EVENT("bake prefiltered");

    auto& gm = IGraphicsManager::GetSingleton();
    gm.SetPipelineState(PSO_PREFILTER);
    auto [width, height] = p_framebuffer->GetBufferSize();

    auto skybox = gm.FindTexture(RESOURCE_ENV_SKYBOX_CUBE_MAP);
    DEV_ASSERT(skybox);
    gm.BindTexture(Dimension::TEXTURE_CUBE, skybox->GetHandle(), GetSkyboxSlot());
    auto& frame = gm.GetCurrentFrame();
    for (int mip_idx = 0; mip_idx < IBL_MIP_CHAIN_MAX; ++mip_idx, width /= 2, height /= 2) {
        for (int face_id = 0; face_id < 6; ++face_id) {
            const int index = mip_idx * 6 + face_id;
            gm.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), index);

            gm.SetRenderTarget(p_framebuffer, face_id, mip_idx);
            gm.SetViewport(Viewport(width, height));
            gm.DrawSkybox();
        }
    }
    gm.UnbindTexture(Dimension::TEXTURE_CUBE, GetSkyboxSlot());
}

void RenderGraphBuilder::AddGenerateSkylightPass() {
    RenderPassCreateInfo info{
        .name = RenderPassName::ENV,
        .drawPasses = {
            { { { RESOURCE_ENV_SKYBOX_CUBE_MAP } }, ConvertToCubemapFunc },
            { { { RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP } }, DiffuseIrradianceFunc },
            { { { RESOURCE_ENV_PREFILTER_CUBE_MAP } }, PrefilteredFunc },
        },
    };

    CreateRenderPass(info);
}

static void PathTracerTonePassFunc(const RenderData&, const Framebuffer* p_framebuffer, DrawPass&,
                                   IRenderCmdContext&) {
    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    gm.SetRenderTarget(p_framebuffer);

    auto depth_buffer = p_framebuffer->desc.depthAttachment;
    const auto [width, height] = p_framebuffer->GetBufferSize();

    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<GpuTexture> resource = gm.FindTexture(p_name);
        if (!resource) {
            return;
        }

        gm.BindTexture(p_dimension, resource->GetHandle(), p_slot);
    };
    bind_slot(RESOURCE_PATH_TRACER, GetBloomInputTextureSlot());

    gm.SetViewport(Viewport(width, height));
    gm.Clear(p_framebuffer, CLEAR_COLOR_BIT);

    gm.SetPipelineState(PSO_TONE);
    gm.DrawQuad();

    gm.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

void RenderGraphBuilder::AddPathTracerTonePass() {
    auto& gm = IGraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::TONE;

    desc.dependencies = { RenderPassName::PATH_TRACER };

    auto pass = m_graph.CreatePass(desc);

    auto attachment = gm.FindTexture(RESOURCE_TONE);

    FramebufferDesc framebuffer_desc{ .colorAttachments = { attachment } };

    if (m_config.enableHighlight) {
        auto gbuffer_depth = gm.FindTexture(RESOURCE_GBUFFER_DEPTH);
        framebuffer_desc.depthAttachment = gbuffer_depth;
    }

    auto framebuffer = gm.CreateFramebuffer(framebuffer_desc);

    pass->AddDrawPass("PathTracerDrawPass", framebuffer, PathTracerTonePassFunc);
}

static void PathTracerPassFunc(const RenderData&, const Framebuffer*, DrawPass&,
                               IRenderCmdContext&) {
    // @TODO: refactor this part
    if (!renderer::IsPathTracerActive()) {
        return;
    }

    auto& gm = IGraphicsManager::GetSingleton();

    gm.SetPipelineState(PSO_PATH_TRACER);
    auto input = gm.FindTexture(RESOURCE_PATH_TRACER);

    DEV_ASSERT(input);

    const uint32_t width = input->desc.width;
    const uint32_t height = input->desc.height;
    const uint32_t work_group_x = CeilingDivision(width, 16);
    const uint32_t work_group_y = CeilingDivision(height, 16);

    // @TODO: transition
    renderer::BindPathTracerData(gm);

    gm.Dispatch(work_group_x, work_group_y, 1);

    renderer::UnbindPathTracerData(gm);
}

void RenderGraphBuilder::AddPathTracerPass() {
    auto& gm = IGraphicsManager::GetSingleton();

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

    auto pass = gm.CreateFramebuffer(FramebufferDesc{
        .transitions = {
            ResourceTransition{
                .resource = resource,
                .slot = GetUavSlotPathTracerOutputImage(),
                .beginPassFunc = [](IGraphicsManager* p_graphics_manager,
                                    GpuTexture* p_texture,
                                    int p_slot) { p_graphics_manager->BindUnorderedAccessView(p_slot, p_texture); },
                .endPassFunc = [](IGraphicsManager* p_graphics_manager,
                                  GpuTexture*,
                                  int p_slot) { p_graphics_manager->UnbindUnorderedAccessView(p_slot); },
            } },
    });
    render_pass->AddDrawPass("PassTracerComputePass", pass, PathTracerPassFunc);
}

/// Create pre-defined passes
std::unique_ptr<RenderGraph> RenderGraphBuilder::CreateEmpty(RenderGraphBuilderConfig& p_config) {
    p_config.enableBloom = false;
    p_config.enableIbl = false;
    p_config.enableVxgi = false;

    auto graph = std::make_unique<RenderGraph>();
    RenderGraphBuilder creator(p_config, *graph.get());

    creator.AddForward();

    graph->Compile();
    return graph;
}

std::unique_ptr<RenderGraph> RenderGraphBuilder::CreateDummy(RenderGraphBuilderConfig& p_config) {
    p_config.enableBloom = false;
    p_config.enableIbl = false;
    p_config.enableVxgi = false;

    auto graph = std::make_unique<RenderGraph>();
    RenderGraphBuilder creator(p_config, *graph.get());

    creator.AddPrepass();
    creator.AddGbufferPass();

    graph->Compile();
    return graph;
}

std::unique_ptr<RenderGraph> RenderGraphBuilder::CreatePathTracer(RenderGraphBuilderConfig& p_config) {
    p_config.enableBloom = false;
    p_config.enableIbl = false;
    p_config.enableVxgi = false;
    p_config.enableHighlight = false;

    auto graph = std::make_unique<RenderGraph>();
    RenderGraphBuilder creator(p_config, *graph.get());

    creator.AddPathTracerPass();
    creator.AddPathTracerTonePass();

    graph->Compile();
    return graph;
}

static void RgCreateResource(IGraphicsManager& p_manager,
                             const SamplerDesc& p_sampler,
                             RenderTargetResourceName p_name,
                             PixelFormat p_format,
                             AttachmentType p_type,
                             int p_width,
                             int p_height,
                             int p_depth = 1,
                             ResourceMiscFlags p_misc_flag = RESOURCE_MISC_NONE,
                             uint32_t p_mips_level = 0) {
    int width = p_width;
    int height = p_height;
    if (p_width < 0) {
        switch (p_width) {
            case RESOURCE_SIZE_FRAME: {
                const Vector2i frame_size = DVAR_GET_IVEC2(resolution);
                width = frame_size.x;
                height = frame_size.y;
            } break;
            default: {
                CRASH_NOW();
            } break;
        }
    }
    DEV_ASSERT(width > 0 && height > 0);

    auto desc = RenderGraphBuilder::BuildDefaultTextureDesc(p_name, p_format, p_type, width, height, p_depth, p_misc_flag, p_mips_level);
    [[maybe_unused]] auto ret = p_manager.CreateTexture(desc, p_sampler);
    DEV_ASSERT(ret);
}

void RenderGraphBuilder::CreateResources() {
    auto& manager = IGraphicsManager::GetSingleton();
#define RG_RESOURCE(NAME, SAMPLER, FORMAT, TYPE, W, ...) \
    RgCreateResource(manager, SAMPLER, NAME, FORMAT, TYPE, W, __VA_ARGS__);
#include "engine/renderer/render_graph/render_graph_resources.h"
}

std::unique_ptr<RenderGraph> RenderGraphBuilder::CreateDefault(RenderGraphBuilderConfig& p_config) {
    p_config.enableBloom = true;
    p_config.enableIbl = false;
    p_config.enableVxgi = IGraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;

    auto graph = std::make_unique<RenderGraph>();
    RenderGraphBuilder creator(p_config, *graph.get());

    creator.AddPrepass();
    creator.AddGbufferPass();
    creator.AddGenerateSkylightPass();
    creator.AddShadowPass();
    creator.AddSsaoPass();
    creator.AddHighlightPass();
    creator.AddVoxelizationPass();
    creator.AddLightingPass();
    creator.AddSkyPass();
    creator.AddBloomPass();
    creator.AddTonePass();
    creator.AddDebugImagePass();

    graph->Compile();
    return graph;
}

GpuTextureDesc RenderGraphBuilder::BuildDefaultTextureDesc(RenderTargetResourceName p_name,
                                                           PixelFormat p_format,
                                                           AttachmentType p_type,
                                                           uint32_t p_width,
                                                           uint32_t p_height,
                                                           uint32_t p_array_size,
                                                           ResourceMiscFlags p_misc_flag,
                                                           uint32_t p_mips_level) {
    GpuTextureDesc desc{};
    desc.type = p_type;
    desc.name = p_name;
    desc.format = p_format;
    desc.arraySize = p_array_size;
    desc.dimension = Dimension::TEXTURE_2D;
    desc.width = p_width;
    desc.height = p_height;
    desc.mipLevels = p_mips_level ? p_mips_level : 1;
    desc.miscFlags = p_misc_flag;
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
