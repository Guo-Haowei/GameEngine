#include "common_passes.h"

#include "engine/algorithm/algorithm.h"
#include "engine/assets/asset.h"
#include "engine/core/base/random.h"
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
#include "engine/renderer/ltc_matrix.h"
#include "engine/runtime/asset_registry.h"

namespace my {
#include "shader_resource_defines.hlsl.h"
}  // namespace my

namespace my {

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

static void ExecuteDrawCommands(const RenderSystem& p_data, const RenderPass& p_draw_pass, bool p_is_prepass = false) {

    HBN_PROFILE_EVENT();

    auto& gm = IGraphicsManager::GetSingleton();
    auto& frame = gm.GetCurrentFrame();
    for (const RenderCommand& cmd : p_draw_pass.GetCommands()) {
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

struct ScopedEvent {
    IRenderCmdContext& m_ctx;

    ScopedEvent(IRenderCmdContext& p_ctx, std::string_view p_name) : m_ctx(p_ctx) {
        m_ctx.BeginEvent(p_name);
    }

    ~ScopedEvent() {
        m_ctx.EndEvent();
    }
};

#define RENDER_PASS_FUNC()                                \
    ScopedEvent _scoped(p_ctx.cmd, p_ctx.pass.GetName()); \
    HBN_PROFILE_EVENT();

static void EarlyZPassFunc(RenderPassExcutionContext& p_ctx) {
    RENDER_PASS_FUNC();

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

void RenderGraphBuilderExt::AddEmpty() {
    auto color_desc = BuildDefaultTextureDesc(DEFAULT_SURFACE_FORMAT,
                                              AttachmentType::COLOR_2D);

    auto depth_desc = BuildDefaultTextureDesc(RT_FMT_GBUFFER_DEPTH,
                                              AttachmentType::DEPTH_STENCIL_2D);

    auto& pass = AddPass(RG_PASS_EMPTY);
    pass.Create(RG_RES_OVERLAY, { color_desc })
        .Create(RG_RES_DEPTH_STENCIL, { depth_desc })
        .Write(ResourceAccess::RTV, RG_RES_OVERLAY)
        .Write(ResourceAccess::DSV, RG_RES_DEPTH_STENCIL)
        .SetExecuteFunc(EmptyPass);
}

void RenderGraphBuilderExt::AddEarlyZPass() {
    auto buffer_desc = BuildDefaultTextureDesc(RT_FMT_GBUFFER_DEPTH,
                                               AttachmentType::DEPTH_STENCIL_2D);

    auto& pass = AddPass(RG_PASS_EARLY_Z);
    pass.Create(RG_RES_DEPTH_STENCIL, { buffer_desc })
        .Write(ResourceAccess::DSV, RG_RES_DEPTH_STENCIL)
        .SetExecuteFunc(EarlyZPassFunc);
}

static void GbufferPassFunc(RenderPassExcutionContext& p_ctx) {
    RENDER_PASS_FUNC();

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

void RenderGraphBuilderExt::AddGbufferPass() {
    auto color0_desc = BuildDefaultTextureDesc(RT_FMT_GBUFFER_BASE_COLOR,
                                               AttachmentType::COLOR_2D);

    auto color1_desc = BuildDefaultTextureDesc(RT_FMT_GBUFFER_NORMAL,
                                               AttachmentType::COLOR_2D);

    auto color2_desc = BuildDefaultTextureDesc(RT_FMT_GBUFFER_MATERIAL,
                                               AttachmentType::COLOR_2D);

    auto& pass = AddPass(RG_PASS_GBUFFER);
    pass.Create(RG_RES_GBUFFER_COLOR0, { color0_desc })
        .Create(RG_RES_GBUFFER_COLOR1, { color1_desc })
        .Create(RG_RES_GBUFFER_COLOR2, { color2_desc })
        .Write(ResourceAccess::DSV, RG_RES_DEPTH_STENCIL)
        .Write(ResourceAccess::RTV, RG_RES_GBUFFER_COLOR0)
        .Write(ResourceAccess::RTV, RG_RES_GBUFFER_COLOR1)
        .Write(ResourceAccess::RTV, RG_RES_GBUFFER_COLOR2)
        .SetExecuteFunc(GbufferPassFunc);
}

// textures generated by program
static std::shared_ptr<GpuTexture> GenerateSsaoNoise() {
    // generate noise texture
    std::vector<Vector2f> ssao_noise;
    for (int i = 0; i < (SSAO_NOISE_SIZE * SSAO_NOISE_SIZE); ++i) {
        Vector2f noise(Random::Float(-1.0f, 1.0f),
                       Random::Float(-1.0f, 1.0f));
        ssao_noise.emplace_back(noise);
    }

    GpuTextureDesc desc{
        .type = AttachmentType::NONE,
        .dimension = Dimension::TEXTURE_2D,
        .width = SSAO_NOISE_SIZE,
        .height = SSAO_NOISE_SIZE,
        .depth = 1,
        .mipLevels = 1,
        .arraySize = 1,
        .format = PixelFormat::R32G32_FLOAT,
        .bindFlags = BIND_SHADER_RESOURCE,
        .miscFlags = RESOURCE_MISC_NONE,
        .initialData = ssao_noise.data(),
        .name = RG_RES_SSAO,
    };

    return IGraphicsManager::GetSingleton().CreateTexture(desc, PointWrapSampler());
}

static void SsaoPassFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.options.ssaoEnabled) {
        return;
    }

    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;

    auto fb = p_ctx.framebuffer;
    const uint32_t width = fb->desc.colorAttachments[0]->desc.width;
    const uint32_t height = fb->desc.colorAttachments[0]->desc.height;

    cmd.SetRenderTarget(fb);
    cmd.SetViewport(Viewport(width, height));
    cmd.Clear(fb, CLEAR_COLOR_BIT);

    {
        // @TODO: get rid of this
        // should not use this, instead, save projection view matrices in framecb
        const PassContext& pass = p_ctx.render_system.mainPass;
        cmd.BindConstantBufferSlot<PerPassConstantBuffer>(cmd.GetCurrentFrame().passCb.get(), pass.pass_idx);
    }

    cmd.SetPipelineState(PSO_SSAO);
    cmd.DrawQuad();
}

void RenderGraphBuilderExt::AddSsaoPass() {
    auto color0_desc = BuildDefaultTextureDesc(RT_FMT_SSAO,
                                               AttachmentType::COLOR_2D);

    auto& pass = AddPass(RG_PASS_SSAO);
    pass.Create(RG_RES_SSAO, { color0_desc })
        .Import(RG_RES_SSAO_NOISE, []() {
            return GenerateSsaoNoise();
        })
        .Write(ResourceAccess::RTV, RG_RES_SSAO)
        .Read(ResourceAccess::SRV, RG_RES_GBUFFER_COLOR1)
        .Read(ResourceAccess::SRV, RG_RES_DEPTH_STENCIL)
        .Read(ResourceAccess::SRV, RG_RES_SSAO_NOISE)
        .SetExecuteFunc(SsaoPassFunc);
}

static void HighlightPassFunc(RenderPassExcutionContext& p_ctx) {
    RENDER_PASS_FUNC();

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

void RenderGraphBuilderExt::AddHighlightPass() {
    auto color0_desc = BuildDefaultTextureDesc(RT_FMT_OUTLINE_SELECT,
                                               AttachmentType::COLOR_2D);

    auto& pass = AddPass(RG_PASS_OUTLINE);
    pass.Create(RG_RES_OUTLINE, { color0_desc })
        .Write(ResourceAccess::RTV, RG_RES_OUTLINE)
        .Write(ResourceAccess::DSV, RG_RES_DEPTH_STENCIL)
        .SetExecuteFunc(HighlightPassFunc);
}

/// Shadow
[[maybe_unused]] static void PointShadowPassFunc(RenderPassExcutionContext& p_ctx) {
    RENDER_PASS_FUNC();

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
    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;

    const Framebuffer* framebuffer = p_ctx.framebuffer;
    const auto& frame = cmd.GetCurrentFrame();

    cmd.SetRenderTarget(framebuffer);
    const auto [width, height] = framebuffer->GetBufferSize();

    cmd.Clear(framebuffer, CLEAR_DEPTH_BIT);

    cmd.SetViewport(Viewport(width, height));

    const PassContext& pass = p_ctx.render_system.shadowPasses[0];
    cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

    cmd.SetPipelineState(PSO_DPETH);
    ExecuteDrawCommands(p_ctx.render_system, p_ctx.pass);
}

void RenderGraphBuilderExt::AddShadowPass() {
    const int shadow_res = DVAR_GET_INT(gfx_shadow_res);
    DEV_ASSERT(IsPowerOfTwo(shadow_res));

    GpuTextureDesc shadow_map_desc = BuildDefaultTextureDesc(PixelFormat::D32_FLOAT,
                                                             AttachmentType::SHADOW_2D,
                                                             1 * shadow_res, shadow_res);
    auto& pass = AddPass(RG_PASS_SHADOW);
    pass.Create(RG_RES_SHADOW_MAP, { shadow_map_desc, ShadowMapSampler() })
        .Write(ResourceAccess::DSV, RG_RES_SHADOW_MAP)
        .SetExecuteFunc(ShadowPassFunc);
}

static void VoxelizationPassFunc(RenderPassExcutionContext& p_ctx) {
    if (p_ctx.render_system.voxelPass.pass_idx < 0) {
        return;
    }

    RENDER_PASS_FUNC();
    auto& cmd = p_ctx.cmd;
    const auto& frame = cmd.GetCurrentFrame();

    const int voxel_size = DVAR_GET_INT(gfx_voxel_size);

    // post process
    const uint32_t group_size = voxel_size / COMPUTE_LOCAL_SIZE_VOXEL;
    cmd.SetPipelineState(PSO_VOXELIZATION_PRE);
    cmd.Dispatch(group_size, group_size, group_size);

    const PassContext& pass = p_ctx.render_system.voxelPass;
    cmd.BindConstantBufferSlot<PerPassConstantBuffer>(frame.passCb.get(), pass.pass_idx);

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

    for (auto& uav : p_ctx.pass.GetUavs()) {
        cmd.GenerateMipmap(uav.get());
    }

    // @TODO: [SCRUM-28] refactor
    cmd.UnsetRenderTarget();
}

void RenderGraphBuilderExt::AddVoxelizationPass() {
    auto& manager = IGraphicsManager::GetSingleton();
    if (manager.GetBackend() != Backend::OPENGL) {
        return;
    }

    const int voxel_size = DVAR_GET_INT(gfx_voxel_size);
    GpuTextureDesc desc = BuildDefaultTextureDesc(PixelFormat::R16G16B16A16_FLOAT,
                                                  AttachmentType::RW_TEXTURE,
                                                  voxel_size, voxel_size);
    desc.dimension = Dimension::TEXTURE_3D;
    desc.mipLevels = LogTwo(voxel_size);
    desc.depth = voxel_size;
    desc.miscFlags |= RESOURCE_MISC_GENERATE_MIPS;

    SamplerDesc sampler(MinFilter::LINEAR_MIPMAP_LINEAR, MagFilter::POINT, AddressMode::BORDER);

    auto& pass = AddPass(RG_PASS_VOXELIZATION);
    pass.Create(RG_RES_VOXEL_LIGHTING, { desc, sampler })
        .Create(RG_RES_VOXEL_NORMAL, { desc, sampler })
        .Read(ResourceAccess::SRV, RG_RES_SHADOW_MAP)
        //.Read(ResourceAccess::SRV, RG_RES_LTC1)
        //.Read(ResourceAccess::SRV, RG_RES_LTC2)
        .Read(ResourceAccess::UAV, RG_RES_VOXEL_LIGHTING)
        .Read(ResourceAccess::UAV, RG_RES_VOXEL_NORMAL)
        .SetExecuteFunc(VoxelizationPassFunc);
}

/// Emitter
static void EmitterPassFunc(RenderPassExcutionContext& p_ctx) {
    RENDER_PASS_FUNC();

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
    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;
    const auto [width, height] = fb->GetBufferSize();

    cmd.SetRenderTarget(fb);

    cmd.SetViewport(Viewport(width, height));
    cmd.Clear(fb, CLEAR_COLOR_BIT);
    cmd.SetPipelineState(PSO_LIGHTING);

    cmd.DrawQuad();
}

static std::shared_ptr<GpuTexture> GenerateLTC(std::string_view p_name, const float* p_matrix_table) {
    constexpr int LTC_SIZE = 64;
    GpuTextureDesc desc{
        .type = AttachmentType::NONE,
        .dimension = Dimension::TEXTURE_2D,
        .width = LTC_SIZE,
        .height = LTC_SIZE,
        .depth = 1,
        .mipLevels = 1,
        .arraySize = 1,
        .format = PixelFormat::R32G32B32A32_FLOAT,
        .bindFlags = BIND_SHADER_RESOURCE,
        .miscFlags = RESOURCE_MISC_NONE,
        .initialData = p_matrix_table,
        .name = std::string(p_name),
    };

    return IGraphicsManager::GetSingleton().CreateTexture(desc, PointClampSampler());
}

// @TODO: refactor
#if 0
static unsigned int LoadMTexture(const float* matrixTable) {
    unsigned int texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_FLOAT, matrixTable);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}
#endif

void RenderGraphBuilderExt::AddLightingPass() {
    auto lighting_desc = BuildDefaultTextureDesc(RT_FMT_LIGHTING,
                                                 AttachmentType::COLOR_2D);

    auto& pass = AddPass(RG_PASS_LIGHTING);
    // @TODO: dynamic
    pass.Import(RG_RES_BRDF, []() {
            auto image = AssetRegistry::GetSingleton().GetAssetByHandle<ImageAsset>(AssetHandle{ "@res://images/brdf.hdr" });
            return IGraphicsManager::GetSingleton().CreateTexture(const_cast<ImageAsset*>(image));
        })
        .Import(RG_RES_LTC1, []() {
            return GenerateLTC(RG_RES_LTC1, LTC1);
        })
        .Import(RG_RES_LTC2, []() {
            return GenerateLTC(RG_RES_LTC2, LTC2);
        })
        .Create(RG_RES_LIGHTING, { lighting_desc })
        .Write(ResourceAccess::RTV, RG_RES_LIGHTING)
        .Read(ResourceAccess::SRV, RG_RES_GBUFFER_COLOR0)
        .Read(ResourceAccess::SRV, RG_RES_GBUFFER_COLOR1)
        .Read(ResourceAccess::SRV, RG_RES_GBUFFER_COLOR2)
        .Read(ResourceAccess::SRV, RG_RES_DEPTH_STENCIL)
        .Read(ResourceAccess::SRV, RG_RES_SSAO)
        .Read(ResourceAccess::SRV, RG_RES_SHADOW_MAP)
        .Read(ResourceAccess::SRV, RG_RES_ENV_DIFFUSE_CUBE)
        .Read(ResourceAccess::SRV, RG_RES_ENV_PREFILTERED_CUBE)
        .Read(ResourceAccess::SRV, RG_RES_BRDF)
        .Read(ResourceAccess::SRV, RG_RES_LTC1)
        .Read(ResourceAccess::SRV, RG_RES_LTC2)
        .SetExecuteFunc(LightingPassFunc);

    if (m_config.enableVxgi) {
        pass.Read(ResourceAccess::SRV, RG_RES_VOXEL_LIGHTING)
            .Read(ResourceAccess::SRV, RG_RES_VOXEL_NORMAL);
    }
}

/// Sky
static void ForwardPassFunc(RenderPassExcutionContext& p_ctx) {
    RENDER_PASS_FUNC();

    auto& gm = p_ctx.cmd;

    auto fb = p_ctx.framebuffer;
    const auto [width, height] = fb->GetBufferSize();

    gm.SetRenderTarget(fb);

    gm.SetViewport(Viewport(width, height));

    const PassContext& pass = p_ctx.render_system.mainPass;
    gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.GetCurrentFrame().passCb.get(), pass.pass_idx);

    gm.SetPipelineState(PSO_ENV_SKYBOX);
    gm.SetStencilRef(STENCIL_FLAG_SKY);
    gm.DrawSkybox();
    gm.SetStencilRef(0);

    // draw transparent objects
    gm.SetPipelineState(PSO_FORWARD_TRANSPARENT);
    ExecuteDrawCommands(p_ctx.render_system, p_ctx.pass);

    auto& draw_context = p_ctx.render_system.drawDebugContext;
    if (gm.m_debugBuffers && draw_context.drawCount) {
        gm.BindConstantBufferSlot<PerPassConstantBuffer>(gm.GetCurrentFrame().passCb.get(), pass.pass_idx);
        gm.SetPipelineState(PSO_DEBUG_DRAW);

        gm.UpdateBuffer(CreateDesc(draw_context.positions),
                        gm.m_debugBuffers->vertexBuffers[0].get());
        gm.UpdateBuffer(CreateDesc(draw_context.colors),
                        gm.m_debugBuffers->vertexBuffers[6].get());
        gm.SetMesh(gm.m_debugBuffers.get());
        gm.DrawArrays(draw_context.drawCount);
    }

    EmitterPassFunc(p_ctx);
}

void RenderGraphBuilderExt::AddForwardPass() {
    auto& pass = AddPass(RG_PASS_FORWARD);
    AddDependency(RG_PASS_LIGHTING, RG_PASS_FORWARD);
    pass.Read(ResourceAccess::SRV, RG_RES_ENV_SKYBOX_CUBE)
        .Read(ResourceAccess::SRV, RG_RES_SHADOW_MAP)
        .Read(ResourceAccess::SRV, RG_RES_ENV_DIFFUSE_CUBE)
        .Read(ResourceAccess::SRV, RG_RES_ENV_PREFILTERED_CUBE)
        .Read(ResourceAccess::SRV, RG_RES_BRDF)
        .Read(ResourceAccess::SRV, RG_RES_LTC1)
        .Read(ResourceAccess::SRV, RG_RES_LTC2)
        .Write(ResourceAccess::DSV, RG_RES_DEPTH_STENCIL)
        .Write(ResourceAccess::RTV, RG_RES_LIGHTING)
        .SetExecuteFunc(ForwardPassFunc);

    if (m_config.enableVxgi) {
        pass.Read(ResourceAccess::SRV, RG_RES_VOXEL_LIGHTING)
            .Read(ResourceAccess::SRV, RG_RES_VOXEL_NORMAL);
    }
}

/// Bloom
static void BloomSetupFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.options.bloomEnabled) {
        return;
    }

    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;

    auto uav = p_ctx.pass.GetUavs()[0];

    cmd.SetPipelineState(PSO_BLOOM_SETUP);

    const uint32_t work_group_x = CeilingDivision(uav->desc.width, 16);
    const uint32_t work_group_y = CeilingDivision(uav->desc.height, 16);

    cmd.Dispatch(work_group_x, work_group_y, 1);
}

static void BloomDownSampleFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.options.bloomEnabled) {
        return;
    }

    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;

    cmd.SetPipelineState(PSO_BLOOM_DOWNSAMPLE);

    auto uav = p_ctx.pass.GetUavs()[0];

    const uint32_t work_group_x = CeilingDivision(uav->desc.width, 16);
    const uint32_t work_group_y = CeilingDivision(uav->desc.height, 16);

    cmd.Dispatch(work_group_x, work_group_y, 1);
}

static void BloomUpSampleFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.options.bloomEnabled) {
        return;
    }

    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;

    auto uav = p_ctx.pass.GetUavs()[0];

    cmd.SetPipelineState(PSO_BLOOM_UPSAMPLE);

    const uint32_t work_group_x = CeilingDivision(uav->desc.width, 16);
    const uint32_t work_group_y = CeilingDivision(uav->desc.height, 16);

    cmd.Dispatch(work_group_x, work_group_y, 1);
}

void RenderGraphBuilderExt::AddBloomPass() {
    // Setup pass
    const int width = m_config.frameWidth;
    const int height = m_config.frameHeight;

    auto& setup_pass = AddPass(RG_PASS_BLOOM_SETUP);
    SamplerDesc sampler = LinearClampSampler();

    // @TODO: use mips instead of generate this many resources
    for (int i = 0, w = width, h = height; i < BLOOM_MIP_CHAIN_MAX; ++i, w /= 2, h /= 2) {
        DEV_ASSERT(width > 1);
        DEV_ASSERT(height > 1);

        auto texture_desc = BuildDefaultTextureDesc(PixelFormat::R16G16B16A16_FLOAT,
                                                    AttachmentType::COLOR_2D,
                                                    w, h);

        auto res_name = std::format(RG_RES_BLOOM_PREFIX "{}x{}", w, h);
        setup_pass.Create(res_name, { texture_desc, sampler });
    }

    auto bloom_res = std::format(RG_RES_BLOOM_PREFIX "{}x{}", width, height);

    AddDependency(RG_PASS_FORWARD, RG_PASS_BLOOM_SETUP);
    setup_pass
        .Read(ResourceAccess::SRV, RG_RES_LIGHTING)
        .Read(ResourceAccess::UAV, bloom_res)
        .SetExecuteFunc(BloomSetupFunc);

    // Down Sample
    for (int i = 0, w = width, h = height; i < BLOOM_MIP_CHAIN_MAX - 1; ++i, w /= 2, h /= 2) {
        auto pass_name = std::format(RG_PASS_BLOOM_DOWN_PREFIX "{}", i);
        auto input = std::format(RG_RES_BLOOM_PREFIX "{}x{}", w, h);
        auto inout = std::format(RG_RES_BLOOM_PREFIX "{}x{}", w / 2, h / 2);
        auto& pass = AddPass(pass_name);
        pass.Read(ResourceAccess::SRV, input)
            .Read(ResourceAccess::UAV, inout)
            .SetExecuteFunc(BloomDownSampleFunc);
        if (i == 0) {
            AddDependency(RG_PASS_BLOOM_SETUP, pass_name);
        } else {
            std::string prev_pass = std::format(RG_PASS_BLOOM_DOWN_PREFIX "{}", i - 1);
            AddDependency(prev_pass, pass_name);
        }
    }

    // Up Sample
    for (int i = 0, w = width, h = height; i < BLOOM_MIP_CHAIN_MAX - 1; ++i, w /= 2, h /= 2) {
        auto pass_name = std::format(RG_PASS_BLOOM_UP_PREFIX "{}", i);
        auto mip_low = std::format(RG_RES_BLOOM_PREFIX "{}x{}", w / 2, h / 2);
        auto mip = std::format(RG_RES_BLOOM_PREFIX "{}x{}", w, h);

        auto& pass = AddPass(pass_name);
        pass.Read(ResourceAccess::UAV, mip)
            .Read(ResourceAccess::SRV, mip_low)
            .SetExecuteFunc(BloomUpSampleFunc);

        if (i == BLOOM_MIP_CHAIN_MAX - 2) {
            auto down_sample_pass = std::format(RG_PASS_BLOOM_DOWN_PREFIX "{}", BLOOM_MIP_CHAIN_MAX - 2);
            AddDependency(down_sample_pass, pass_name);
        } else {
            auto prev_pass = std::format(RG_PASS_BLOOM_UP_PREFIX "{}", i + 1);
            AddDependency(prev_pass, pass_name);
        }
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
    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;
    cmd.SetRenderTarget(fb);

    auto depth_buffer = fb->desc.depthAttachment;
    const auto [width, height] = fb->GetBufferSize();

    // draw billboards

    // @HACK:
    if (DVAR_GET_BOOL(gfx_debug_vxgi) && cmd.GetBackend() == Backend::OPENGL) {
        // @TODO: add to forward pass
        DebugVoxels(p_ctx.render_system, fb);
    } else {
        cmd.SetViewport(Viewport(width, height));
        cmd.Clear(fb, CLEAR_COLOR_BIT);

        cmd.SetPipelineState(PSO_POST_PROCESS);
        cmd.DrawQuad();
    }
}

void RenderGraphBuilderExt::AddTonePass() {
    auto desc = BuildDefaultTextureDesc(RT_FMT_TONE,
                                        AttachmentType::COLOR_2D);

    auto bloom_res = std::format(RG_RES_BLOOM_PREFIX "{}x{}", m_config.frameWidth, m_config.frameHeight);

    AddDependency(RG_PASS_BLOOM_UP_PREFIX "0", RG_PASS_POST_PROCESS);
    auto& pass = AddPass(RG_PASS_POST_PROCESS);
    pass.Create(RG_RES_POST_PROCESS, { desc })
        .Read(ResourceAccess::SRV, RG_RES_LIGHTING)
        .Read(ResourceAccess::SRV, RG_RES_OUTLINE)
        .Read(ResourceAccess::SRV, bloom_res);

    if (m_config.enableVxgi) {
        // @TODO: move the debug to somewhere else
        pass.Read(ResourceAccess::UAV, RG_RES_VOXEL_LIGHTING)
            .Read(ResourceAccess::UAV, RG_RES_VOXEL_NORMAL);
    }

    pass.Write(ResourceAccess::RTV, RG_RES_POST_PROCESS)
        .Write(ResourceAccess::DSV, RG_RES_DEPTH_STENCIL)
        .SetExecuteFunc(TonePassFunc);
}

// assume render target is setup
static void DrawDebugImages(const RenderSystem& p_data, int p_width, int p_height, IGraphicsManager& p_graphics_manager) {
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

static void DebugImagesFunc(RenderPassExcutionContext& p_ctx) {
    HBN_PROFILE_EVENT();
    auto fb = p_ctx.framebuffer;
    auto& cmd = p_ctx.cmd;
    cmd.SetRenderTarget(fb);
    cmd.Clear(fb, CLEAR_COLOR_BIT);

    const int width = fb->desc.colorAttachments[0]->desc.width;
    const int height = fb->desc.colorAttachments[0]->desc.height;
    DrawDebugImages(p_ctx.render_system, width, height, cmd);
}

void RenderGraphBuilderExt::AddDebugImagePass() {
    if (m_config.is_runtime) {
        return;
    }

    auto desc = BuildDefaultTextureDesc(DEFAULT_SURFACE_FORMAT,
                                        AttachmentType::COLOR_2D);
    desc.bindFlags |= BIND_SHADER_RESOURCE;

    auto& pass = AddPass(RG_PASS_OVERLAY);
    pass.Create(RG_RES_OVERLAY, { desc })
        .Read(ResourceAccess::SRV, RG_RES_POST_PROCESS)
        .Write(ResourceAccess::RTV, RG_RES_OVERLAY)
        .SetExecuteFunc(DebugImagesFunc);
}

static void ConvertToCubemapFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.bakeIbl) {
        return;
    }

    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;

    cmd.SetPipelineState(PSO_ENV_SKYBOX_TO_CUBE_MAP);
    auto cube_map = fb->desc.colorAttachments[0];
    const auto [width, height] = fb->GetBufferSize();

    auto& frame = cmd.GetCurrentFrame();
    for (int i = 0; i < 6; ++i) {
        cmd.SetRenderTarget(fb, i);

        cmd.SetViewport(Viewport(width, height));

        cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
        cmd.DrawSkybox();
    }
    cmd.GenerateMipmap(cube_map.get());
}

static void DiffuseIrradianceFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.bakeIbl) {
        return;
    }

    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;

    cmd.SetPipelineState(PSO_DIFFUSE_IRRADIANCE);
    const auto [width, height] = fb->GetBufferSize();

    auto& frame = cmd.GetCurrentFrame();
    for (int i = 0; i < 6; ++i) {
        cmd.SetRenderTarget(fb, i);
        cmd.SetViewport(Viewport(width, height));

        cmd.BindConstantBufferSlot<PerBatchConstantBuffer>(frame.batchCb.get(), i);
        cmd.DrawSkybox();
    }
}

static void PrefilteredFunc(RenderPassExcutionContext& p_ctx) {
    if (!p_ctx.render_system.bakeIbl) {
        return;
    }

    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;

    cmd.SetPipelineState(PSO_PREFILTER);
    auto [width, height] = fb->GetBufferSize();

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
}

void RenderGraphBuilderExt::AddGenerateSkylightPass() {
    {
        GpuTextureDesc desc = BuildDefaultTextureDesc(PixelFormat::R32G32B32A32_FLOAT,
                                                      AttachmentType::COLOR_CUBE,
                                                      RT_SIZE_IBL_CUBEMAP,
                                                      RT_SIZE_IBL_CUBEMAP,
                                                      6,
                                                      RESOURCE_MISC_GENERATE_MIPS,
                                                      IBL_MIP_CHAIN_MAX);

        auto& pass = AddPass(RG_PASS_BAKE_SKYBOX);
        pass.Import(RG_RES_IBL, []() {
                AssetHandle handle{ "@res://images/ibl/circus.hdr" };
                auto image = AssetRegistry::GetSingleton().GetAssetByHandle<ImageAsset>(handle);
                return IGraphicsManager::GetSingleton().CreateTexture(const_cast<ImageAsset*>(image));
            })
            .Create(RG_RES_ENV_SKYBOX_CUBE, { desc, CubemapSampler() })
            .Read(ResourceAccess::SRV, RG_RES_IBL)
            .Write(ResourceAccess::RTV, RG_RES_ENV_SKYBOX_CUBE)
            .SetExecuteFunc(ConvertToCubemapFunc);
    }
    {
        GpuTextureDesc desc = BuildDefaultTextureDesc(PixelFormat::R32G32B32A32_FLOAT,
                                                      AttachmentType::COLOR_CUBE,
                                                      RT_SIZE_IBL_IRRADIANCE_CUBEMAP,
                                                      RT_SIZE_IBL_IRRADIANCE_CUBEMAP,
                                                      6);

        auto& pass = AddPass(RG_PASS_BAKE_DIFFUSE);
        pass.Create(RG_RES_ENV_DIFFUSE_CUBE, { desc, CubemapNoMipSampler() })
            .Read(ResourceAccess::SRV, RG_RES_ENV_SKYBOX_CUBE)
            .Write(ResourceAccess::RTV, RG_RES_ENV_DIFFUSE_CUBE)
            .SetExecuteFunc(DiffuseIrradianceFunc);
    }
    {
        GpuTextureDesc desc = BuildDefaultTextureDesc(PixelFormat::R32G32B32A32_FLOAT,
                                                      AttachmentType::COLOR_CUBE,
                                                      RT_SIZE_IBL_PREFILTERED_CUBEMAP,
                                                      RT_SIZE_IBL_PREFILTERED_CUBEMAP,
                                                      6,
                                                      RESOURCE_MISC_GENERATE_MIPS,
                                                      IBL_MIP_CHAIN_MAX);

        auto& pass = AddPass(RG_PASS_BAKE_PREFILTERED);
        pass.Create(RG_RES_ENV_PREFILTERED_CUBE, { desc, CubemapLodSampler() })
            .Read(ResourceAccess::SRV, RG_RES_ENV_SKYBOX_CUBE)
            .Write(ResourceAccess::RTV, RG_RES_ENV_PREFILTERED_CUBE)
            .SetExecuteFunc(PrefilteredFunc);

        AddDependency(RG_PASS_BAKE_PREFILTERED, RG_PASS_EARLY_Z);
    }
}

static void PathTracerPassFunc(RenderPassExcutionContext& p_ctx) {
    // @TODO: refactor this part
    if (!renderer::IsPathTracerActive()) {
        return;
    }

    auto& cmd = p_ctx.cmd;

    cmd.SetPipelineState(PSO_PATH_TRACER);
    const auto& input = p_ctx.pass.GetUavs()[0];

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

void RenderGraphBuilderExt::AddPathTracerPass() {
    GpuTextureDesc texture_desc = BuildDefaultTextureDesc(PixelFormat::R32G32B32A32_FLOAT,
                                                          AttachmentType::COLOR_2D);

    auto& pass = AddPass(RG_PASS_PATHTRACER);
    pass.Create(RG_RES_PATHTRACER, { texture_desc, LinearClampSampler() })
        .Read(ResourceAccess::UAV, RG_RES_PATHTRACER)
        .SetExecuteFunc(PathTracerPassFunc);
}

static void PathTracerTonePassFunc(RenderPassExcutionContext& p_ctx) {
    RENDER_PASS_FUNC();

    auto& cmd = p_ctx.cmd;
    auto fb = p_ctx.framebuffer;
    cmd.SetRenderTarget(fb);

    auto depth_buffer = fb->desc.depthAttachment;
    const auto [width, height] = fb->GetBufferSize();

    cmd.SetViewport(Viewport(width, height));
    cmd.Clear(fb, CLEAR_COLOR_BIT);

    cmd.SetPipelineState(PSO_POST_PROCESS);
    cmd.DrawQuad();
}

void RenderGraphBuilderExt::AddPathTracerTonePass() {
    auto& pass = AddPass(RG_PASS_PATHTRACER_PRESENT);

    pass.Import(RG_RES_POST_PROCESS, []() {
            return IGraphicsManager::GetSingleton().FindTexture(RG_RES_POST_PROCESS);
        })
        .Read(ResourceAccess::SRV, RG_RES_PATHTRACER)
        .Write(ResourceAccess::RTV, RG_RES_POST_PROCESS)
        .Write(ResourceAccess::DSV, RG_RES_DEPTH_STENCIL)
        .SetExecuteFunc(PathTracerTonePassFunc);
}

/// Create pre-defined passes
auto RenderGraphBuilderExt::CreateEmpty(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>> {
    p_config.enableBloom = false;
    p_config.enableIbl = false;
    p_config.enableVxgi = false;

    RenderGraphBuilderExt builder(p_config);
    builder.AddEmpty();
    return builder.Compile();
}

auto RenderGraphBuilderExt::CreateDefault(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>> {
    p_config.enableBloom = true;
    p_config.enableIbl = false;
    p_config.enableVxgi = IGraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;

    RenderGraphBuilderExt builder(p_config);

    builder.AddEarlyZPass();
    builder.AddGbufferPass();
    builder.AddGenerateSkylightPass();
    builder.AddShadowPass();
    builder.AddSsaoPass();
    builder.AddHighlightPass();
    builder.AddVoxelizationPass();
    builder.AddLightingPass();
    builder.AddForwardPass();
    builder.AddBloomPass();
    builder.AddTonePass();
    builder.AddDebugImagePass();

    return builder.Compile();
}

auto RenderGraphBuilderExt::CreatePathTracer(RenderGraphBuilderConfig& p_config) -> Result<std::shared_ptr<RenderGraph>> {
    p_config.enableBloom = false;
    p_config.enableIbl = false;
    p_config.enableVxgi = false;
    p_config.enableHighlight = false;

    RenderGraphBuilderExt creator(p_config);

    creator.AddPathTracerPass();
    creator.AddPathTracerTonePass();

    return creator.Compile();
}

GpuTextureDesc RenderGraphBuilderExt::BuildDefaultTextureDesc(PixelFormat p_format,
                                                              AttachmentType p_type,
                                                              uint32_t p_width,
                                                              uint32_t p_height,
                                                              uint32_t p_array_size,
                                                              ResourceMiscFlags p_misc_flag,
                                                              uint32_t p_mips_level) {
    GpuTextureDesc desc{};
    desc.type = p_type;
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
    return desc;
};

}  // namespace my
