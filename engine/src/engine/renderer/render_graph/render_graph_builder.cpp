#include "render_graph_builder.h"

#include "engine/assets/asset.h"
#include "engine/core/debugger/profiler.h"
#include "engine/math/matrix_transform.h"
#include "engine/renderer/base_graphics_manager.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/renderer/render_system.h"
#include "engine/renderer/renderer_misc.h"
#include "engine/renderer/sampler.h"
#include "engine/runtime/display_manager.h"
#include "render_graph_defines.h"
#include "render_pass_builder.h"

// @TODO: remove
#include "engine/runtime/asset_registry.h"

// @TODO: refactor
#define GBUFFER_PASS_NAME                "GbufferDrawPass"
#define FORWARD_PASS_NAME                "ForwardDrawPass"
#define VOXELIZATION_PASS_NAME           "VoxelizationDrawPass"
#define EARLY_Z_PASS_NAME                "EarlyZDrawPass"
#define SHADOW_DRAW_PASS_NAME            "ShadowDrawPass"
#define POINT_SHADOW_DRAW_PASS_NAME(IDX) "ShadowDrawPass" #IDX

// @TODO: refactor
#define RG_PASS_EARLY_Z      "p:early_z"
#define RG_PASS_SHADOW       "p:shadow"
#define RG_PASS_GBUFFER      "p:gbuffer"
#define RG_PASS_VOXELIZATION "p:voxelization"

#define RG_RES_DEPTH          "r:depth"
#define RG_RES_SHADOW_MAP     "r:shadow"
#define RG_RES_GBUFFER_COLOR0 "r:gbuffer0"
#define RG_RES_GBUFFER_COLOR1 "r:gbuffer1"
#define RG_RES_GBUFFER_COLOR2 "r:gbuffer2"
#define RG_RES_VOXEL_LIGHTING "r:voxel_lighting"
#define RG_RES_VOXEL_NORMAL   "r:voxel_normal"

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
    ExecuteFunc func{ nullptr };
};

struct RenderPassCreateInfo {
    RenderPassName name;
    std::vector<RenderPassName> dependencies;
    std::vector<DrawPassCreateInfo> drawPasses;
};

/// Gbuffer

// @TODO: generalize this
static void DrawInstacedGeometry(const RenderSystem& p_data, const std::vector<InstanceContext>& p_instances, bool p_is_prepass) {
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

static void ExecuteDrawCommands(const RenderSystem& p_data, const DrawPass& p_draw_pass, bool p_is_prepass = false) {

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

static void EarlyZPassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    Framebuffer* fb = p_ctx.framebuffer;
    auto& cmd = p_ctx.cmd;
    auto& frame = cmd.GetCurrentFrame();
    const uint32_t width = fb->desc.depthAttachment->desc.width;
    const uint32_t height = fb->desc.depthAttachment->desc.height;

    cmd.SetRenderTarget(fb);
    cmd.SetViewport(Viewport(width, height));

    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmd.Clear(fb, CLEAR_DEPTH_BIT | CLEAR_STENCIL_BIT, clear_color, 0.0f, STENCIL_FLAG_SKY);

    const PassContext& pass = p_ctx.render_system.mainPass;
    cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    cmd.SetPipelineState(PSO_PREPASS);
    ExecuteDrawCommands(p_ctx.render_system, p_ctx.pass, true);
}

static void EmptyPass(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();
    auto& cmd = p_ctx.cmd;
    cmd.SetRenderTarget(p_ctx.framebuffer);
    float clear_color[] = { 0.3f, 0.5f, 0.4f, 1.0f };
    cmd.Clear(p_ctx.framebuffer, CLEAR_COLOR_BIT | CLEAR_DEPTH_BIT, clear_color);
}

void RenderGraphBuilder::AddEmpty() {
    RenderPassCreateInfo info{
        .name = RenderPassName::FINAL,
        .drawPasses = {
            { {
                  { RESOURCE_FINAL },
                  RESOURCE_GBUFFER_DEPTH,
              },
              EmptyPass },
        }
    };

    CreateRenderPass(info);
}

void RenderGraphBuilder::AddEarlyZPass() {
    auto& manager = IGraphicsManager::GetSingleton();
    const Vector2i frame_size = DVAR_GET_IVEC2(resolution);

    std::shared_ptr<GpuTexture> depth;
    {
        auto desc = BuildDefaultTextureDesc(RESOURCE_GBUFFER_DEPTH,
                                            RT_FMT_GBUFFER_DEPTH,
                                            AttachmentType::DEPTH_STENCIL_2D,
                                            frame_size.x, frame_size.y);

        depth = manager.CreateTexture(desc, PointClampSampler());

        AddPass(RG_PASS_EARLY_Z)
            .Create(RG_RES_DEPTH, desc)
            .SetExecuteFunc(EarlyZPassFunc);
    }

    RenderPassDesc desc;
    desc.name = RenderPassName::PREPASS;
    auto pass = m_graph.CreatePass(desc);

    FramebufferDesc fb_desc;
    fb_desc.depthAttachment = depth;

    auto framebuffer = manager.CreateFramebuffer(fb_desc);
    pass->AddDrawPass(EARLY_Z_PASS_NAME, framebuffer, EarlyZPassFunc);
}

static void GbufferPassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    const Framebuffer* fb = p_ctx.framebuffer;
    auto& cmd = p_ctx.cmd;

    const auto& frame = cmd.GetCurrentFrame();
    const uint32_t width = fb->desc.depthAttachment->desc.width;
    const uint32_t height = fb->desc.depthAttachment->desc.height;

    cmd.SetRenderTarget(fb);
    cmd.SetViewport(Viewport(width, height));

    const float clear_color[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    cmd.Clear(fb, CLEAR_COLOR_BIT, clear_color);

    const PassContext& pass = p_ctx.render_system.mainPass;
    cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    cmd.SetPipelineState(PSO_GBUFFER);
    ExecuteDrawCommands(p_ctx.render_system, p_ctx.pass, false);
    DrawInstacedGeometry(p_ctx.render_system, p_ctx.render_system.instances, false);
    cmd.SetPipelineState(PSO_GBUFFER_DOUBLE_SIDED);
}

void RenderGraphBuilder::AddGbufferPass() {
    GpuTextureDesc dummy{};

    AddPass(RG_PASS_GBUFFER)
        .Read(RG_RES_DEPTH)
        .Create(RG_RES_GBUFFER_COLOR0, dummy)
        .Create(RG_RES_GBUFFER_COLOR1, dummy)
        .Create(RG_RES_GBUFFER_COLOR2, dummy)
        .SetExecuteFunc(GbufferPassFunc);

    auto& manager = IGraphicsManager::GetSingleton();

    auto color0 = manager.FindTexture(RESOURCE_GBUFFER_BASE_COLOR);
    auto color1 = manager.FindTexture(RESOURCE_GBUFFER_NORMAL);
    auto color2 = manager.FindTexture(RESOURCE_GBUFFER_MATERIAL);
    auto depth = manager.FindTexture(RESOURCE_GBUFFER_DEPTH);

    RenderPassDesc desc;
    desc.dependencies = { RenderPassName::PREPASS },
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

static void SsaoPassFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.options.ssaoEnabled) {
        return;
    }

    HBN_PROFILE_EVENT();

    auto& cmd = p_ctx.cmd;
    auto& frame = cmd.GetCurrentFrame();
    auto fb = p_ctx.framebuffer;
    const uint32_t width = fb->desc.colorAttachments[0]->desc.width;
    const uint32_t height = fb->desc.colorAttachments[0]->desc.height;

    cmd.SetRenderTarget(fb);
    cmd.SetViewport(Viewport(width, height));
    cmd.Clear(fb, CLEAR_COLOR_BIT);

    const PassContext& pass = p_ctx.render_system.mainPass;
    cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    auto noise = renderer::GetSsaoNoiseTexture();
    cmd.BindTexture(Dimension::TEXTURE_2D, noise->GetHandle(), 0);
    cmd.SetPipelineState(PSO_SSAO);
    cmd.DrawQuad();
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

static void HighlightPassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    auto fb = p_ctx.framebuffer;
    auto& cmd = p_ctx.cmd;
    cmd.SetRenderTarget(fb);
    const auto [width, height] = fb->GetBufferSize();

    cmd.SetViewport(Viewport(width, height));

    cmd.SetPipelineState(PSO_HIGHLIGHT);
    cmd.SetStencilRef(STENCIL_FLAG_SELECTED);
    cmd.Clear(fb, CLEAR_COLOR_BIT);
    cmd.DrawQuad();
    cmd.SetStencilRef(0);
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
static void PointShadowPassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    auto& cmd = p_ctx.cmd;
    auto framebuffer = p_ctx.framebuffer;

    auto& frame = cmd.GetCurrentFrame();

    // prepare render data
    const auto [width, height] = framebuffer->GetBufferSize();

    for (int pass_id = 0; pass_id < MAX_POINT_LIGHT_SHADOW_COUNT; ++pass_id) {
        auto& pass_ptr = p_ctx.render_system.pointShadowPasses[pass_id];
        if (!pass_ptr) {
            continue;
        }

        for (int face_id = 0; face_id < 6; ++face_id) {
            const uint32_t slot = pass_id * 6 + face_id;
            cmd.BindConstantBufferSlot<PointShadowConstantBuffer>(frame.pointShadowCb.get(), slot);

            cmd.SetRenderTarget(framebuffer, slot);
            cmd.Clear(framebuffer, CLEAR_DEPTH_BIT, nullptr, 1.0f, 0, slot);

            cmd.SetViewport(Viewport(width, height));

            cmd.SetPipelineState(PSO_POINT_SHADOW);
            ExecuteDrawCommands(p_ctx.render_system, p_ctx.pass, false);
        }
    }
}

static void ShadowPassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    const Framebuffer* framebuffer = p_ctx.framebuffer;
    auto& cmd = p_ctx.cmd;
    const auto& frame = cmd.GetCurrentFrame();

    cmd.SetRenderTarget(framebuffer);
    const auto [width, height] = framebuffer->GetBufferSize();

    cmd.Clear(framebuffer, CLEAR_DEPTH_BIT);

    cmd.SetViewport(Viewport(width, height));

    const PassContext& pass = p_ctx.render_system.shadowPasses[0];
    cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    auto draw_batches = [&](const std::vector<BatchContext>& p_batches) {
        for (const auto& draw : p_batches) {
            const bool has_bone = draw.bone_idx >= 0;
            if (has_bone) {
                cmd.BindConstantBufferSlot<BoneConstantBuffer>(frame.boneCb.get(), draw.bone_idx);
            }

            cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), draw.batch_idx);

            cmd.SetMesh(draw.mesh_data);
            cmd.DrawElements(draw.mesh_data->desc.drawCount);
        }
    };
    cmd.SetPipelineState(PSO_DPETH);
    ExecuteDrawCommands(p_ctx.render_system, p_ctx.pass);
}

void RenderGraphBuilder::AddShadowPass() {
    IGraphicsManager& manager = IGraphicsManager::GetSingleton();

    const int shadow_res = DVAR_GET_INT(gfx_shadow_res);
    DEV_ASSERT(IsPowerOfTwo(shadow_res));
    const int point_shadow_res = DVAR_GET_INT(gfx_point_shadow_res);
    DEV_ASSERT(IsPowerOfTwo(point_shadow_res));

    GpuTextureDesc dummy{};

    AddPass(RG_PASS_SHADOW)
        .Create(RG_RES_SHADOW_MAP, dummy)
        .SetExecuteFunc(ShadowPassFunc);

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

static void VoxelizationPassFunc(RenderPassExcutionContext& p_ctx) {
    if (p_ctx.render_system.voxelPass.pass_idx < 0) {
        return;
    }

    HBN_PROFILE_EVENT();
    auto& cmd = p_ctx.cmd;
    const auto& frame = cmd.GetCurrentFrame();

    auto voxel_lighting = cmd.FindTexture(RESOURCE_VOXEL_LIGHTING);
    auto voxel_normal = cmd.FindTexture(RESOURCE_VOXEL_NORMAL);
    DEV_ASSERT(voxel_lighting && voxel_normal);

    // @TODO: refactor pass to auto bind resources,
    // and make it a class so don't do a map search every frame
    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<GpuTexture> resource = cmd.FindTexture(p_name);
        if (!resource) {
            return;
        }

        cmd.BindTexture(p_dimension, resource->GetHandle(), p_slot);
    };

    // bind common textures
    bind_slot(RESOURCE_SHADOW_MAP, GetShadowMapSlot());
    bind_slot(RESOURCE_POINT_SHADOW_CUBE_ARRAY, GetPointShadowArraySlot(), Dimension::TEXTURE_CUBE_ARRAY);

    const int voxel_size = DVAR_GET_INT(gfx_voxel_size);

    cmd.BindUnorderedAccessView(IMAGE_VOXEL_ALBEDO_SLOT, voxel_lighting.get());
    cmd.BindUnorderedAccessView(IMAGE_VOXEL_NORMAL_SLOT, voxel_normal.get());

    // post process
    const uint32_t group_size = voxel_size / COMPUTE_LOCAL_SIZE_VOXEL;
    cmd.SetPipelineState(PSO_VOXELIZATION_PRE);
    cmd.Dispatch(group_size, group_size, group_size);

    const PassContext& pass = p_ctx.render_system.voxelPass;
    cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    // glSubpixelPrecisionBiasNV(1, 1);
    // glSubpixelPrecisionBiasNV(8, 8);

    // @TODO: hack
    if (cmd.GetBackend() == Backend::OPENGL) {
        cmd.SetViewport(Viewport(voxel_size, voxel_size));
        cmd.SetPipelineState(PSO_VOXELIZATION);
        cmd.SetBlendState(PipelineStateManager::GetBlendDescDisable(), nullptr, 0xFFFFFFFF);
        ExecuteDrawCommands(p_ctx.render_system, p_ctx.pass);

        // glSubpixelPrecisionBiasNV(0, 0);
        cmd.SetBlendState(PipelineStateManager::GetBlendDescDefault(), nullptr, 0xFFFFFFFF);
    }

    // post process
    cmd.SetPipelineState(PSO_VOXELIZATION_POST);
    cmd.Dispatch(group_size, group_size, group_size);

    cmd.GenerateMipmap(voxel_lighting.get());
    cmd.GenerateMipmap(voxel_normal.get());

    // unbind stuff
    cmd.UnbindTexture(Dimension::TEXTURE_2D, GetShadowMapSlot());
    cmd.UnbindTexture(Dimension::TEXTURE_CUBE_ARRAY, GetPointShadowArraySlot());

    // @TODO: [SCRUM-28] refactor
    cmd.UnsetRenderTarget();
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

    GpuTextureDesc dummy{};
    AddPass(RG_PASS_VOXELIZATION)
        .Read(RG_RES_SHADOW_MAP)
        .Create(RG_RES_VOXEL_LIGHTING, dummy)
        .Create(RG_RES_VOXEL_NORMAL, dummy)
        .SetExecuteFunc(VoxelizationPassFunc);

    // @TODO: ? build

    RenderPassDesc desc;
    desc.name = RenderPassName::VOXELIZATION;
    desc.dependencies = { RenderPassName::SHADOW };
    auto pass = m_graph.CreatePass(desc);
    auto framebuffer = manager.CreateFramebuffer(FramebufferDesc{});
    pass->AddDrawPass(VOXELIZATION_PASS_NAME, framebuffer, VoxelizationPassFunc);
}

/// Emitter
static void EmitterPassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;
    auto& render_system = p_ctx.render_system;
    auto& frame = cmd.GetCurrentFrame();
    const auto [width, height] = fb->GetBufferSize();

    cmd.SetRenderTarget(fb);
    cmd.SetViewport(Viewport(width, height));

    const PassContext& pass = render_system.mainPass;
    cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    int particle_idx = 0;
    for (const auto& emitter : render_system.emitters) {
        if (!emitter.particleBuffer) {
            continue;
        }

        cmd.BindConstantBufferSlot<EmitterConstantBuffer>(frame.emitterCb.get(), particle_idx);
        ++particle_idx;

        cmd.BindStructuredBuffer(GetGlobalParticleCounterSlot(), emitter.counterBuffer.get());
        cmd.BindStructuredBuffer(GetGlobalDeadIndicesSlot(), emitter.deadBuffer.get());
        cmd.BindStructuredBuffer(GetGlobalAliveIndicesPreSimSlot(), emitter.aliveBuffer[emitter.GetPreIndex()].get());
        cmd.BindStructuredBuffer(GetGlobalAliveIndicesPostSimSlot(), emitter.aliveBuffer[emitter.GetPostIndex()].get());
        cmd.BindStructuredBuffer(GetGlobalParticleDataSlot(), emitter.particleBuffer.get());

        cmd.SetPipelineState(PSO_PARTICLE_KICKOFF);
        cmd.Dispatch(1, 1, 1);

        cmd.SetPipelineState(PSO_PARTICLE_EMIT);
        cmd.Dispatch(MAX_PARTICLE_COUNT / PARTICLE_LOCAL_SIZE, 1, 1);

        cmd.SetPipelineState(PSO_PARTICLE_SIM);
        cmd.Dispatch(MAX_PARTICLE_COUNT / PARTICLE_LOCAL_SIZE, 1, 1);

        cmd.UnbindStructuredBuffer(GetGlobalParticleCounterSlot());
        cmd.UnbindStructuredBuffer(GetGlobalDeadIndicesSlot());
        cmd.UnbindStructuredBuffer(GetGlobalAliveIndicesPreSimSlot());
        cmd.UnbindStructuredBuffer(GetGlobalAliveIndicesPostSimSlot());
        cmd.UnbindStructuredBuffer(GetGlobalParticleDataSlot());

        // Renderering
        cmd.SetPipelineState(PSO_PARTICLE_RENDERING);

        bool use_texture = false;
        if (!emitter.texture.empty()) {
            const ImageAsset* image = AssetRegistry::GetSingleton().GetAssetByHandle<ImageAsset>(emitter.texture);
            if (image && image->gpu_texture) {
                cmd.BindTexture(Dimension::TEXTURE_2D, image->gpu_texture->GetHandle(), GetBaseColorMapSlot());
                use_texture = true;
            }
        }

        cmd.BindStructuredBufferSRV(GetGlobalParticleDataSlot(), emitter.particleBuffer.get());
        cmd.DrawQuadInstanced(MAX_PARTICLE_COUNT);
        cmd.UnbindStructuredBufferSRV(GetGlobalParticleDataSlot());

        if (use_texture) {
            cmd.UnbindTexture(Dimension::TEXTURE_2D, GetBaseColorMapSlot());
        }
    }
}

/// Lighting
static void LightingPassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;
    const auto [width, height] = fb->GetBufferSize();

    cmd.SetRenderTarget(fb);

    cmd.SetViewport(Viewport(width, height));
    cmd.Clear(fb, CLEAR_COLOR_BIT);
    cmd.SetPipelineState(PSO_LIGHTING);

    const auto brdf = cmd.m_brdfImage->gpu_texture;
    auto diffuse_iraddiance = cmd.FindTexture(RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP);
    auto prefiltered = cmd.FindTexture(RESOURCE_ENV_PREFILTER_CUBE_MAP);
    DEV_ASSERT(brdf && diffuse_iraddiance && prefiltered);

    // @TODO: fix
    auto voxel_lighting = cmd.FindTexture(RESOURCE_VOXEL_LIGHTING);
    auto voxel_normal = cmd.FindTexture(RESOURCE_VOXEL_NORMAL);
    if (voxel_lighting && voxel_normal) {
        cmd.BindTexture(Dimension::TEXTURE_3D, voxel_lighting->GetHandle(), GetVoxelLightingSlot());
        cmd.BindTexture(Dimension::TEXTURE_3D, voxel_normal->GetHandle(), GetVoxelNormalSlot());
    }

    cmd.BindTexture(Dimension::TEXTURE_2D, brdf->GetHandle(), GetBrdfLutSlot());
    cmd.BindTexture(Dimension::TEXTURE_CUBE, diffuse_iraddiance->GetHandle(), GetDiffuseIrradianceSlot());
    cmd.BindTexture(Dimension::TEXTURE_CUBE, prefiltered->GetHandle(), GetPrefilteredSlot());

    cmd.DrawQuad();

    cmd.UnbindTexture(Dimension::TEXTURE_2D, GetBrdfLutSlot());
    cmd.UnbindTexture(Dimension::TEXTURE_CUBE, GetDiffuseIrradianceSlot());
    cmd.UnbindTexture(Dimension::TEXTURE_CUBE, GetPrefilteredSlot());

    if (voxel_lighting && voxel_normal) {
        cmd.UnbindTexture(Dimension::TEXTURE_3D, GetVoxelLightingSlot());
        cmd.UnbindTexture(Dimension::TEXTURE_3D, GetVoxelNormalSlot());
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
static void ForwardPassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    auto& gm = p_ctx.cmd;

    auto fb = p_ctx.framebuffer;
    const auto [width, height] = fb->GetBufferSize();

    gm.SetRenderTarget(fb);

    gm.SetViewport(Viewport(width, height));

    const PassContext& pass = p_ctx.render_system.mainPass;
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
    ExecuteDrawCommands(p_ctx.render_system, p_ctx.pass);

    auto& draw_context = p_ctx.render_system.drawDebugContext;
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

    EmitterPassFunc(p_ctx);
}

void RenderGraphBuilder::AddForwardPass() {
    auto& manager = IGraphicsManager::GetSingleton();

    auto gbuffer_depth = manager.FindTexture(RESOURCE_GBUFFER_DEPTH);

    auto lighting_attachment = manager.FindTexture(RESOURCE_LIGHTING);

    RenderPassDesc desc;
    desc.name = RenderPassName::FORWARD;

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
    pass->AddDrawPass(FORWARD_PASS_NAME, framebuffer, ForwardPassFunc);
}

/// Bloom
static void BloomSetupFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.options.bloomEnabled) {
        return;
    }

    HBN_PROFILE_EVENT();

    auto& cmd = p_ctx.cmd;
    auto& frame = cmd.GetCurrentFrame();

    cmd.SetPipelineState(PSO_BLOOM_SETUP);
    auto input = cmd.FindTexture(RESOURCE_LIGHTING);

    const uint32_t width = input->desc.width;
    const uint32_t height = input->desc.height;
    const uint32_t work_group_x = CeilingDivision(width, 16);
    const uint32_t work_group_y = CeilingDivision(height, 16);

    cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), 0);

    cmd.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), GetBloomInputTextureSlot());
    cmd.Dispatch(work_group_x, work_group_y, 1);
    cmd.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

static void BloomDownSampleFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.options.bloomEnabled) {
        return;
    }

    HBN_PROFILE_EVENT();

    const uint32_t pass_id = p_ctx.framebuffer->id;
    auto& cmd = p_ctx.cmd;
    auto& frame = cmd.GetCurrentFrame();

    cmd.SetPipelineState(PSO_BLOOM_DOWNSAMPLE);
    auto input = cmd.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + pass_id));

    DEV_ASSERT(input);

    const uint32_t width = input->desc.width;
    const uint32_t height = input->desc.height;
    const uint32_t work_group_x = CeilingDivision(width, 16);
    const uint32_t work_group_y = CeilingDivision(height, 16);

    cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), pass_id + 1);

    cmd.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), GetBloomInputTextureSlot());
    cmd.Dispatch(work_group_x, work_group_y, 1);
    cmd.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

static void BloomUpSampleFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.options.bloomEnabled) {
        return;
    }

    HBN_PROFILE_EVENT();

    auto& cmd = p_ctx.cmd;
    const uint32_t pass_id = p_ctx.framebuffer->id;
    auto& frame = cmd.GetCurrentFrame();

    cmd.SetPipelineState(PSO_BLOOM_UPSAMPLE);
    auto input = cmd.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + pass_id));
    auto output = cmd.FindTexture(static_cast<RenderTargetResourceName>(RESOURCE_BLOOM_0 + pass_id - 1));

    const uint32_t width = output->desc.width;
    const uint32_t height = output->desc.height;
    const uint32_t work_group_x = CeilingDivision(width, 16);
    const uint32_t work_group_y = CeilingDivision(height, 16);

    cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), pass_id + BLOOM_MIP_CHAIN_MAX);

    cmd.BindTexture(Dimension::TEXTURE_2D, input->GetHandle(), GetBloomInputTextureSlot());
    cmd.Dispatch(work_group_x, work_group_y, 1);
    cmd.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
}

void RenderGraphBuilder::AddBloomPass() {
    auto& gm = IGraphicsManager::GetSingleton();

    RenderPassDesc desc;
    desc.name = RenderPassName::BLOOM;
    desc.dependencies = { RenderPassName::FORWARD };
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

static void DebugVoxels(const RenderSystem& p_data, const Framebuffer* p_framebuffer) {
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
static void TonePassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;
    cmd.SetRenderTarget(fb);

    auto depth_buffer = fb->desc.depthAttachment;
    const auto [width, height] = fb->GetBufferSize();

    // draw billboards

    // @HACK:
    if (DVAR_GET_BOOL(gfx_debug_vxgi) && cmd.GetBackend() == Backend::OPENGL) {
        // @TODO: remove
        DebugVoxels(p_ctx.render_system, fb);
    } else {
        auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
            std::shared_ptr<GpuTexture> resource = cmd.FindTexture(p_name);
            if (!resource) {
                return;
            }

            cmd.BindTexture(p_dimension, resource->GetHandle(), p_slot);
        };
        bind_slot(RESOURCE_LIGHTING, GetTextureLightingSlot());
        bind_slot(RESOURCE_OUTLINE_SELECT, GetTextureHighlightSelectSlot());
        bind_slot(RESOURCE_BLOOM_0, GetBloomInputTextureSlot());

        cmd.SetViewport(Viewport(width, height));
        cmd.Clear(fb, CLEAR_COLOR_BIT);

        cmd.SetPipelineState(PSO_TONE);
        cmd.DrawQuad();

        cmd.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
        cmd.UnbindTexture(Dimension::TEXTURE_2D, GetTextureLightingSlot());
        cmd.UnbindTexture(Dimension::TEXTURE_2D, GetTextureHighlightSelectSlot());
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
void RenderGraphBuilder::DrawDebugImages(const RenderSystem& p_data, int p_width, int p_height, IGraphicsManager& p_graphics_manager) {
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
              [](RenderPassExcutionContext& p_ctx) {
                  HBN_PROFILE_EVENT();
                  auto fb = p_ctx.framebuffer;
                  auto& cmd = p_ctx.cmd;
                  cmd.SetRenderTarget(fb);
                  cmd.Clear(fb, CLEAR_COLOR_BIT);

                  const int width = fb->desc.colorAttachments[0]->desc.width;
                  const int height = fb->desc.colorAttachments[0]->desc.height;
                  DrawDebugImages(p_ctx.render_system, width, height, cmd);
              } },
        },
    };

    CreateRenderPass(info);
}

static void ConvertToCubemapFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT("hdr image to -> skybox");
    if (!p_ctx.render_system.bakeIbl) {
        return;
    }

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;

    cmd.SetPipelineState(PSO_ENV_SKYBOX_TO_CUBE_MAP);
    auto cube_map = fb->desc.colorAttachments[0];
    const auto [width, height] = fb->GetBufferSize();

    auto& frame = cmd.GetCurrentFrame();
    cmd.BindTexture(Dimension::TEXTURE_2D, p_ctx.render_system.skyboxHdr->GetHandle(), GetSkyboxHdrSlot());
    for (int i = 0; i < 6; ++i) {
        cmd.SetRenderTarget(fb, i);

        cmd.SetViewport(Viewport(width, height));

        cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
        cmd.DrawSkybox();
    }
    cmd.UnbindTexture(Dimension::TEXTURE_2D, GetSkyboxHdrSlot());
    cmd.GenerateMipmap(cube_map.get());
}

static void DiffuseIrradianceFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.bakeIbl) {
        return;
    }
    HBN_PROFILE_EVENT("bake diffuse irradiance");

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;

    cmd.SetPipelineState(PSO_DIFFUSE_IRRADIANCE);
    const auto [width, height] = fb->GetBufferSize();

    auto skybox = cmd.FindTexture(RESOURCE_ENV_SKYBOX_CUBE_MAP);
    DEV_ASSERT(skybox);
    cmd.BindTexture(Dimension::TEXTURE_CUBE, skybox->GetHandle(), GetSkyboxSlot());
    auto& frame = cmd.GetCurrentFrame();
    for (int i = 0; i < 6; ++i) {
        cmd.SetRenderTarget(fb, i);
        cmd.SetViewport(Viewport(width, height));

        cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
        cmd.DrawSkybox();
    }
    cmd.UnbindTexture(Dimension::TEXTURE_CUBE, GetSkyboxSlot());
}

static void PrefilteredFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.bakeIbl) {
        return;
    }
    HBN_PROFILE_EVENT("bake prefiltered");

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;

    cmd.SetPipelineState(PSO_PREFILTER);
    auto [width, height] = fb->GetBufferSize();

    auto skybox = cmd.FindTexture(RESOURCE_ENV_SKYBOX_CUBE_MAP);
    DEV_ASSERT(skybox);
    cmd.BindTexture(Dimension::TEXTURE_CUBE, skybox->GetHandle(), GetSkyboxSlot());
    auto& frame = cmd.GetCurrentFrame();
    for (int mip_idx = 0; mip_idx < IBL_MIP_CHAIN_MAX; ++mip_idx, width /= 2, height /= 2) {
        for (int face_id = 0; face_id < 6; ++face_id) {
            const int index = mip_idx * 6 + face_id;
            cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), index);

            cmd.SetRenderTarget(fb, face_id, mip_idx);
            cmd.SetViewport(Viewport(width, height));
            cmd.DrawSkybox();
        }
    }
    cmd.UnbindTexture(Dimension::TEXTURE_CUBE, GetSkyboxSlot());
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

static void PathTracerTonePassFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;
    cmd.SetRenderTarget(fb);

    auto depth_buffer = fb->desc.depthAttachment;
    const auto [width, height] = fb->GetBufferSize();

    auto bind_slot = [&](RenderTargetResourceName p_name, int p_slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<GpuTexture> resource = cmd.FindTexture(p_name);
        if (!resource) {
            return;
        }

        cmd.BindTexture(p_dimension, resource->GetHandle(), p_slot);
    };
    bind_slot(RESOURCE_PATH_TRACER, GetBloomInputTextureSlot());

    cmd.SetViewport(Viewport(width, height));
    cmd.Clear(fb, CLEAR_COLOR_BIT);

    cmd.SetPipelineState(PSO_TONE);
    cmd.DrawQuad();

    cmd.UnbindTexture(Dimension::TEXTURE_2D, GetBloomInputTextureSlot());
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

static void PathTracerPassFunc(RenderPassExcutionContext& p_ctx) {
    // @TODO: refactor this part
    if (!renderer::IsPathTracerActive()) {
        return;
    }

    auto& cmd = p_ctx.cmd;

    cmd.SetPipelineState(PSO_PATH_TRACER);
    auto input = cmd.FindTexture(RESOURCE_PATH_TRACER);

    DEV_ASSERT(input);

    const uint32_t width = input->desc.width;
    const uint32_t height = input->desc.height;
    const uint32_t work_group_x = CeilingDivision(width, 16);
    const uint32_t work_group_y = CeilingDivision(height, 16);

    // @TODO: transition
    renderer::BindPathTracerData(cmd);

    cmd.Dispatch(work_group_x, work_group_y, 1);

    renderer::UnbindPathTracerData(cmd);
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

    creator.AddEmpty();

    graph->Compile();
    return graph;
}

std::unique_ptr<RenderGraph> RenderGraphBuilder::CreateDummy(RenderGraphBuilderConfig& p_config) {
    p_config.enableBloom = false;
    p_config.enableIbl = false;
    p_config.enableVxgi = false;

    auto graph = std::make_unique<RenderGraph>();
    RenderGraphBuilder creator(p_config, *graph.get());

    creator.AddEarlyZPass();
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

    creator.AddEarlyZPass();
    creator.AddGbufferPass();
    creator.AddGenerateSkylightPass();
    creator.AddShadowPass();
    creator.AddSsaoPass();
    creator.AddHighlightPass();
    creator.AddVoxelizationPass();
    creator.AddLightingPass();
    creator.AddForwardPass();
    creator.AddBloomPass();
    creator.AddTonePass();
    creator.AddDebugImagePass();

    creator.Compile();

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

///////////////////////////

RenderPassBuilder& RenderGraphBuilder::AddPass(std::string_view p_pass_name) {
    RenderPassBuilder builder{ p_pass_name };
    m_passes.push_back(builder);
    return m_passes.back();
}

auto RenderGraphBuilder::Compile() -> Result<void> {
    {
        LOG_WARN("dbg");
        int id = 0;
        for (const auto& pass : m_passes) {
            LOG_OK("found pass: {} (id: {})", pass.GetName(), id++);
            LOG_OK("  it reads:");
            for (const auto& read : pass.m_reads) {
                LOG_OK("  -- {}", read);
            }
            LOG_OK("  it writes:");
            for (const auto& write : pass.m_writes) {
                LOG_OK("  -- {}", write);
            }
        }
    }

    std::unordered_map<std::string_view, int> inputs;
    std::unordered_map<std::string_view, int> outputs;

    const int N = static_cast<int>(m_passes.size());
    for (int i = 0; i < N; ++i) {
        for (const auto& read : m_passes[i].m_reads) {
            inputs.insert(std::make_pair(read, i));
        }
        for (const auto& write : m_passes[i].m_writes) {
            outputs.insert(std::make_pair(write, i));
        }
    }

    // @TODO: figure out dependencies
    // @TODO: validate the graph (duplicate pass? duplicate create? circle?)
    for (const auto& [name, to] : inputs) {
        auto it = outputs.find(name);
        if (it == outputs.end()) continue;

        const int from = it->second;
        LOG_OK("edge found from {} to {}", from, to);
    }

    return Result<void>();
}

}  // namespace my::renderer
