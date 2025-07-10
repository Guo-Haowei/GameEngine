#include "pipeline_state_manager.h"

#include "engine/renderer/graphics_manager.h"
#include "engine/renderer/pipeline_state_objects.h"

namespace my {

const BlendDesc& PipelineStateManager::GetBlendDescDefault() {
    return s_blendStateDefault;
}

const BlendDesc& PipelineStateManager::GetBlendDescDisable() {
    return s_blendStateDisable;
}

PipelineState* PipelineStateManager::Find(PipelineStateName p_name) {
    DEV_ASSERT_INDEX(p_name, m_cache.size());
    return m_cache[p_name].get();
}

auto PipelineStateManager::Create(PipelineStateName p_name, const PipelineStateDesc& p_desc) -> Result<void> {
    if (p_desc.cs.empty()) {
        DEV_ASSERT(p_desc.depthStencilDesc);
    }

    ERR_FAIL_COND_V(m_cache[p_name] != nullptr, HBN_ERROR(ErrorCode::ERR_ALREADY_EXISTS, "pipeline already exists"));

    std::shared_ptr<PipelineState> pipeline{};
    switch (p_desc.type) {
        case PipelineStateType::GRAPHICS: {
            DEV_ASSERT(!p_desc.vs.empty());
            DEV_ASSERT(p_desc.rasterizerDesc);
            DEV_ASSERT(p_desc.depthStencilDesc);
            DEV_ASSERT(p_desc.blendDesc);
            auto result = CreateGraphicsPipeline(p_desc);
            if (!result) {
                return HBN_ERROR(result.error());
            }
            pipeline = *result;
        } break;
        case PipelineStateType::COMPUTE: {
            DEV_ASSERT(!p_desc.cs.empty());
            auto result = CreateComputePipeline(p_desc);
            if (!result) {
                return HBN_ERROR(result.error());
            }
            pipeline = *result;
        } break;
        default:
            CRASH_NOW();
            break;
    }

    if (pipeline == nullptr) {
        return HBN_ERROR(ErrorCode::ERR_CANT_CREATE, "failed to create pipeline '{}'", EnumToString(p_name));
    }

    m_cache[p_name] = pipeline;
    return Result<void>();
}

auto PipelineStateManager::Initialize() -> Result<void> {
    auto ok = Result<void>();
    if constexpr (USING(PLATFORM_WASM)) {
        return ok;
    }
    switch (IGraphicsManager::GetSingleton().GetBackend()) {
        case Backend::EMPTY:
        case Backend::METAL:
        case Backend::VULKAN:
            return ok;
        default:
            break;
    }

#define CREATE_PSO(...)                                       \
    do {                                                      \
        if (auto res = Create(__VA_ARGS__); !res) return res; \
    } while (0)

    CREATE_PSO(PSO_SPRITE,
               {
                   .vs = "sprite.vs",
                   .ps = "sprite.ps",
                   .rasterizerDesc = &s_rasterizerFrontFace,
                   .depthStencilDesc = &s_depthReversedStencilEnabled,
                   .inputLayoutDesc = &s_inputLayoutSprite,
                   .blendDesc = &s_transparent,
                   .numRenderTargets = 1,
                   .rtvFormats = { RT_FMT_TONE },
                   .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,  // gbuffer
               });

    CREATE_PSO(PSO_PREPASS,
               {
                   .vs = "mesh.vs",
                   .rasterizerDesc = &s_rasterizerFrontFace,
                   .depthStencilDesc = &s_depthReversedStencilEnabled,
                   .inputLayoutDesc = &s_inputLayoutMesh,
                   .blendDesc = &s_blendStateDefault,
                   .numRenderTargets = 0,
                   .rtvFormats = {},
                   .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,  // gbuffer
               });

    CREATE_PSO(PSO_GBUFFER,
               {
                   .vs = "mesh.vs",
                   .ps = "gbuffer.ps",
                   .rasterizerDesc = &s_rasterizerFrontFace,
                   .depthStencilDesc = &s_depthReversedStencilDisabled,
                   .inputLayoutDesc = &s_inputLayoutMesh,
                   .blendDesc = &s_blendStateDefault,
                   .numRenderTargets = 4,
                   .rtvFormats = { RT_FMT_GBUFFER_BASE_COLOR,
                                   RT_FMT_GBUFFER_POSITION,
                                   RT_FMT_GBUFFER_NORMAL,
                                   RT_FMT_GBUFFER_MATERIAL },
                   .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,  // gbuffer
               });

    CREATE_PSO(PSO_DEBUG_DRAW,
               {
                   .vs = "debug_draw.vs",
                   .ps = "debug_draw.ps",
                   //.primitiveTopology = PrimitiveTopology::LINE,
                   .rasterizerDesc = &s_rasterizerDoubleSided,
                   .depthStencilDesc = &s_depthReversedStencilDisabled,
                   .inputLayoutDesc = &s_inputLayoutMesh,
                   .blendDesc = &s_transparent,
                   .numRenderTargets = 1,
                   .rtvFormats = { RT_FMT_TONE },
                   .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,  // gbuffer
               });

    CREATE_PSO(PSO_GBUFFER_DOUBLE_SIDED,
               {
                   .vs = "mesh.vs",
                   .ps = "gbuffer.ps",
                   .rasterizerDesc = &s_rasterizerDoubleSided,
                   .depthStencilDesc = &s_depthReversedStencilDisabled,
                   .inputLayoutDesc = &s_inputLayoutMesh,
                   .blendDesc = &s_blendStateDefault,
                   .numRenderTargets = 4,
                   .rtvFormats = { RT_FMT_GBUFFER_BASE_COLOR,
                                   RT_FMT_GBUFFER_POSITION,
                                   RT_FMT_GBUFFER_NORMAL,
                                   RT_FMT_GBUFFER_MATERIAL },
                   .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,  // gbuffer
               });

    CREATE_PSO(PSO_FORWARD_TRANSPARENT,
               {
                   .vs = "mesh.vs",
                   .ps = "forward.ps",
                   .rasterizerDesc = &s_rasterizerDoubleSided,
                   .depthStencilDesc = &s_depthReversedStencilDisabled,
                   .inputLayoutDesc = &s_inputLayoutMesh,
                   .blendDesc = &s_transparent,
                   .numRenderTargets = 1,
                   .rtvFormats = { RT_FMT_LIGHTING },
                   .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,  // gbuffer
               });

    CREATE_PSO(PSO_DPETH, {
                              .vs = "shadow.vs",
                              .ps = "depth.ps",
                              .rasterizerDesc = &s_rasterizerBackFace,
                              .depthStencilDesc = &s_depthStencilDefault,
                              .inputLayoutDesc = &s_inputLayoutMesh,
                              .blendDesc = &s_blendStateDefault,
                              .numRenderTargets = 0,
                              .dsvFormat = PixelFormat::D32_FLOAT,
                          });

    CREATE_PSO(PSO_LIGHTING, {
                                 .vs = "screenspace_quad.vs",
                                 .ps = "lighting.ps",
                                 .rasterizerDesc = &s_rasterizerFrontFace,
                                 .depthStencilDesc = &s_depthStencilDisabled,
                                 .inputLayoutDesc = &s_inputLayoutPosition,
                                 .blendDesc = &s_blendStateDefault,
                                 .numRenderTargets = 1,
                                 .rtvFormats = { RT_FMT_LIGHTING },
                                 .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,
                             });

#pragma region PSO_PARTICLE
    CREATE_PSO(PSO_PARTICLE_INIT, { .type = PipelineStateType::COMPUTE, .cs = "particle_initialization.cs" });
    CREATE_PSO(PSO_PARTICLE_KICKOFF, { .type = PipelineStateType::COMPUTE, .cs = "particle_kickoff.cs" });
    CREATE_PSO(PSO_PARTICLE_EMIT, { .type = PipelineStateType::COMPUTE, .cs = "particle_emission.cs" });
    CREATE_PSO(PSO_PARTICLE_SIM, { .type = PipelineStateType::COMPUTE, .cs = "particle_simulation.cs" });
    CREATE_PSO(PSO_PARTICLE_RENDERING, {
                                           .vs = "particle_draw.vs",
                                           .ps = "particle_draw.ps",
                                           .rasterizerDesc = &s_rasterizerDoubleSided,
                                           .depthStencilDesc = &s_depthReversedStencilDisabled,
                                           .inputLayoutDesc = &s_inputLayoutMesh,
                                           .blendDesc = &s_transparent,
                                           .numRenderTargets = 1,
                                           .rtvFormats = { RT_FMT_LIGHTING },
                                           .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,  // gbuffer
                                       });
#pragma endregion PSO_PARTICLE

    CREATE_PSO(PSO_POINT_SHADOW, {
                                     .vs = "shadowmap_point.vs",
                                     .ps = "shadowmap_point.ps",
                                     .rasterizerDesc = &s_rasterizerBackFace,
                                     .depthStencilDesc = &s_depthStencilDefault,
                                     .inputLayoutDesc = &s_inputLayoutMesh,
                                     .blendDesc = &s_blendStateDefault,
                                     .numRenderTargets = 0,
                                     .dsvFormat = PixelFormat::D32_FLOAT,
                                 });

    CREATE_PSO(PSO_HIGHLIGHT, {
                                  .vs = "screenspace_quad.vs",
                                  .ps = "highlight.ps",
                                  .rasterizerDesc = &s_rasterizerFrontFace,
                                  .depthStencilDesc = &s_depthReversedStencilEnabledHighlight,
                                  .inputLayoutDesc = &s_inputLayoutPosition,
                                  .blendDesc = &s_blendStateDefault,
                                  .numRenderTargets = 1,
                                  .rtvFormats = { RT_FMT_OUTLINE_SELECT },
                                  .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,  // gbuffer
                              });

    CREATE_PSO(PSO_SSAO, {
                             .vs = "screenspace_quad.vs",
                             .ps = "ssao.ps",
                             .rasterizerDesc = &s_rasterizerFrontFace,
                             .depthStencilDesc = &s_depthStencilDisabled,
                             .inputLayoutDesc = &s_inputLayoutPosition,
                             .blendDesc = &s_blendStateDefault,
                             .numRenderTargets = 1,
                             .rtvFormats = { RT_FMT_SSAO },
                         });

    CREATE_PSO(PSO_POST_PROCESS, {
                                     .vs = "screenspace_quad.vs",
                                     .ps = "post_process.ps",
                                     .rasterizerDesc = &s_rasterizerFrontFace,
                                     .depthStencilDesc = &s_depthStencilDisabled,
                                     .inputLayoutDesc = &s_inputLayoutPosition,
                                     .blendDesc = &s_blendStateDefault,
                                     .numRenderTargets = 1,
                                     .rtvFormats = { RT_FMT_TONE },
                                     .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,  // gbuffer
                                 });

#pragma region PSO_BLOOM
    CREATE_PSO(PSO_BLOOM_SETUP, { .type = PipelineStateType::COMPUTE, .cs = "bloom_setup.cs" });
    CREATE_PSO(PSO_BLOOM_DOWNSAMPLE, { .type = PipelineStateType::COMPUTE, .cs = "bloom_downsample.cs" });
    CREATE_PSO(PSO_BLOOM_UPSAMPLE, { .type = PipelineStateType::COMPUTE, .cs = "bloom_upsample.cs" });
#pragma endregion PSO_BLOOM

    CREATE_PSO(PSO_ENV_SKYBOX, {
                                   .vs = "skybox.vs",
                                   .ps = "skybox.ps",
                                   .rasterizerDesc = &s_rasterizerFrontFace,
                                   .depthStencilDesc = &s_depthStencilSkybox,
                                   .inputLayoutDesc = &s_inputLayoutMesh,
                                   .blendDesc = &s_blendStateDefault,
                                   .numRenderTargets = 1,
                                   .rtvFormats = { RT_FMT_LIGHTING },
                                   .dsvFormat = PixelFormat::D32_FLOAT_S8X24_UINT,
                               });

#pragma region PSO_ENV
    CREATE_PSO(PSO_ENV_SKYBOX_TO_CUBE_MAP, {
                                               .vs = "cube_map.vs",
                                               .ps = "to_cube_map.ps",
                                               .rasterizerDesc = &s_rasterizerFrontFace,
                                               .depthStencilDesc = &s_depthStencilDefault,
                                               .inputLayoutDesc = &s_inputLayoutMesh,
                                               .blendDesc = &s_blendStateDefault,
                                           });

    CREATE_PSO(PSO_DIFFUSE_IRRADIANCE, {
                                           .vs = "cube_map.vs",
                                           .ps = "diffuse_irradiance.ps",
                                           .rasterizerDesc = &s_rasterizerFrontFace,
                                           .depthStencilDesc = &s_depthStencilDefault,
                                           .inputLayoutDesc = &s_inputLayoutMesh,
                                           .blendDesc = &s_blendStateDefault,
                                       });

    CREATE_PSO(PSO_PREFILTER, {
                                  .vs = "cube_map.vs",
                                  .ps = "prefilter.ps",
                                  .rasterizerDesc = &s_rasterizerFrontFace,
                                  .depthStencilDesc = &s_depthStencilDefault,
                                  .inputLayoutDesc = &s_inputLayoutMesh,
                                  .blendDesc = &s_blendStateDefault,
                              });
#pragma endregion PSO_ENV

    CREATE_PSO(PSO_RW_TEXTURE_2D, {
                                      .vs = "debug_draw_texture.vs",
                                      .ps = "debug_draw_texture.ps",
                                      .rasterizerDesc = &s_rasterizerFrontFace,
                                      .depthStencilDesc = &s_depthStencilDisabled,
                                      .inputLayoutDesc = &s_inputLayoutPosition,
                                      .blendDesc = &s_blendStateDefault,
                                      .numRenderTargets = 1,
                                      .rtvFormats = { DEFAULT_SURFACE_FORMAT },
                                  });

    // @HACK: only support this many shaders
    if (IGraphicsManager::GetSingleton().GetBackend() == Backend::D3D12) {
        return ok;
    }

    CREATE_PSO(PSO_PATH_TRACER, { .type = PipelineStateType::COMPUTE, .cs = "path_tracer.cs" });

    // @HACK: only support this many shaders
    if (IGraphicsManager::GetSingleton().GetBackend() != Backend::OPENGL) {
        return ok;
    }

#pragma region PSO_VOXEL
    // Voxel
    CREATE_PSO(PSO_VOXELIZATION_PRE, { .type = PipelineStateType::COMPUTE, .cs = "voxelization_pre.cs" });
    CREATE_PSO(PSO_VOXELIZATION_POST, { .type = PipelineStateType::COMPUTE, .cs = "voxelization_post.cs" });

    CREATE_PSO(PSO_VOXELIZATION, {
                                     .vs = "voxelization.vs",
                                     .ps = "voxelization.ps",
                                     .gs = "voxelization.gs",
                                     .rasterizerDesc = &s_rasterizerDoubleSided,
                                     .depthStencilDesc = &s_depthStencilDisabled,
                                     .blendDesc = &s_blendStateDisable,
                                 });

    CREATE_PSO(PSO_DEBUG_VOXEL, {
                                    .vs = "visualization.vs",
                                    .ps = "visualization.ps",
                                    .rasterizerDesc = &s_rasterizerFrontFace,
                                    .depthStencilDesc = &s_depthReversedStencilDisabled,
                                    .blendDesc = &s_blendStateDefault,
                                });
#pragma endregion PSO_VOXEL

#if 0
    CREATE_PSO(PSO_BILLBOARD, {
                                  .vs = "billboard.vs",
                                  .ps = "texture.ps",
                                  .rasterizerDesc = &s_rasterizerDoubleSided,
                                  .depthStencilDesc = &s_depthStencilDefault,
                                  .blendDesc = &s_blendStateDefault,
                              });
#endif

#undef CREATE_PSO

    return ok;
}

void PipelineStateManager::Finalize() {
    for (size_t idx = 0; idx < m_cache.size(); ++idx) {
        m_cache[idx].reset();
    }
}

}  // namespace my
