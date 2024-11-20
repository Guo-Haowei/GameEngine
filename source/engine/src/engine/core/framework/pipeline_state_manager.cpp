#include "pipeline_state_manager.h"

#include "core/framework/asset_manager.h"
#include "core/framework/graphics_manager.h"

namespace my {

// @TODO: make these class members
// input layouts
static const InputLayoutDesc s_inputLayoutMesh = {
    .elements = {
        { "POSITION", 0, PixelFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, PixelFormat::R32G32B32_FLOAT, 1, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, PixelFormat::R32G32_FLOAT, 2, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, PixelFormat::R32G32B32_FLOAT, 3, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "BONEINDEX", 0, PixelFormat::R32G32B32A32_SINT, 4, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "BONEWEIGHT", 0, PixelFormat::R32G32B32A32_FLOAT, 5, 0, InputClassification::PER_VERTEX_DATA, 0 },
    }
};

static const InputLayoutDesc s_inputLayoutPosition = {
    .elements = {
        { "POSITION", 0, PixelFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PER_VERTEX_DATA, 0 },
    }
};

static const RasterizerDesc s_rasterizerFrontFace = {
    .fillMode = FillMode::SOLID,
    .cullMode = CullMode::BACK,
    .frontCounterClockwise = true,
};

static const RasterizerDesc s_rasterizerBackFace = {
    .fillMode = FillMode::SOLID,
    .cullMode = CullMode::FRONT,
    .frontCounterClockwise = true,
};

static const RasterizerDesc s_rasterizerDoubleSided = {
    .fillMode = FillMode::SOLID,
    .cullMode = CullMode::NONE,
    .frontCounterClockwise = true,
};

static const DepthStencilDesc s_depthStencilDefault = {
    .depthEnabled = true,
    .depthFunc = ComparisonFunc::LESS_EQUAL,
    .stencilEnabled = false,
};

static const DepthStencilDesc s_noDepthStencil = {
    .depthEnabled = false,
    .depthFunc = ComparisonFunc::NEVER,
    .stencilEnabled = false,
};

static const DepthStencilDesc s_depthStencilHighlight = {
    .depthEnabled = false,
    .depthFunc = ComparisonFunc::LESS_EQUAL,
    .stencilEnabled = true,
    .frontFace = {
        .stencilFunc = ComparisonFunc::EQUAL,
    },
};

static const DepthStencilDesc s_depthStencilGbuffer = {
    .depthEnabled = true,
    .depthFunc = ComparisonFunc::LESS_EQUAL,
    .stencilEnabled = true,
    .frontFace = {
        .stencilPassOp = StencilOp::REPLACE,
        .stencilFunc = ComparisonFunc::ALWAYS,
    },
};

static const DepthStencilDesc s_depthStencilNoTest = {
    .depthEnabled = false,
    .depthFunc = ComparisonFunc::LESS_EQUAL,
    .stencilEnabled = false,
};

PipelineState* PipelineStateManager::Find(PipelineStateName p_name) {
    DEV_ASSERT_INDEX(p_name, m_cache.size());
    return m_cache[p_name].get();
}

bool PipelineStateManager::Create(PipelineStateName p_name, const PipelineStateDesc& p_desc) {
    if (p_desc.cs.empty()) {
        DEV_ASSERT(p_desc.depthStencilDesc);
    }

    ERR_FAIL_COND_V_MSG(m_cache[p_name] != nullptr, false, "pipeline already exists");

    std::shared_ptr<PipelineState> pipeline{};
    switch (p_desc.type) {
        case PipelineStateType::GRAPHICS:
            DEV_ASSERT(!p_desc.vs.empty());
            DEV_ASSERT(p_desc.rasterizerDesc);
            DEV_ASSERT(p_desc.depthStencilDesc);
            // DEV_ASSERT(p_desc.inputLayoutDesc);
            pipeline = CreateGraphicsPipeline(p_desc);
            break;
        case PipelineStateType::COMPUTE:
            DEV_ASSERT(!p_desc.cs.empty());
            pipeline = CreateComputePipeline(p_desc);
            break;
        default:
            CRASH_NOW();
            break;
    }

    ERR_FAIL_COND_V_MSG(pipeline == nullptr, false, std::format("failed to create pipeline '{}'", std::to_underlying(p_name)));
    m_cache[p_name] = pipeline;
    return true;
}

bool PipelineStateManager::Initialize() {
    bool ok = true;
    if (GraphicsManager::GetSingleton().GetBackend() == Backend::METAL) {
        return ok;
    }

    ok = ok && Create(
                   PSO_GBUFFER,
                   {
                       .vs = "mesh.vs",
                       .ps = "gbuffer.ps",
                       .rasterizerDesc = &s_rasterizerFrontFace,
                       .depthStencilDesc = &s_depthStencilGbuffer,
                       .inputLayoutDesc = &s_inputLayoutMesh,
                       .numRenderTargets = 4,
                       .rtvFormats = { RESOURCE_FORMAT_GBUFFER_BASE_COLOR, RESOURCE_FORMAT_GBUFFER_POSITION, RESOURCE_FORMAT_GBUFFER_NORMAL, RESOURCE_FORMAT_GBUFFER_MATERIAL },
                       .dsvFormat = PixelFormat::D24_UNORM_S8_UINT,  // gbuffer
                   });
    ok = ok && Create(PSO_DPETH, {
                                     .vs = "shadow.vs",
                                     .ps = "depth.ps",
                                     .rasterizerDesc = &s_rasterizerBackFace,
                                     .depthStencilDesc = &s_depthStencilDefault,
                                     .inputLayoutDesc = &s_inputLayoutMesh,
                                     .numRenderTargets = 0,
                                     .dsvFormat = PixelFormat::D32_FLOAT,
                                 });

    ok = ok && Create(PSO_LIGHTING, {
                                        .vs = "screenspace_quad.vs",
                                        .ps = "lighting.ps",
                                        .rasterizerDesc = &s_rasterizerFrontFace,
                                        .depthStencilDesc = &s_noDepthStencil,
                                        .inputLayoutDesc = &s_inputLayoutPosition,
                                        .numRenderTargets = 1,
                                        .rtvFormats = { RESOURCE_FORMAT_LIGHTING },
                                        .dsvFormat = PixelFormat::UNKNOWN,
                                    });

    ok = ok && Create(PSO_POINT_SHADOW, {
                                            .vs = "shadowmap_point.vs",
                                            .ps = "shadowmap_point.ps",
                                            .rasterizerDesc = &s_rasterizerBackFace,
                                            .depthStencilDesc = &s_depthStencilDefault,
                                            .inputLayoutDesc = &s_inputLayoutMesh,
                                            .numRenderTargets = 0,
                                            .dsvFormat = PixelFormat::D32_FLOAT,
                                        });

    ok = ok && Create(PSO_HIGHLIGHT, {
                                         .vs = "screenspace_quad.vs",
                                         .ps = "highlight.ps",
                                         .rasterizerDesc = &s_rasterizerFrontFace,
                                         .depthStencilDesc = &s_depthStencilHighlight,
                                         .inputLayoutDesc = &s_inputLayoutPosition,
                                         .numRenderTargets = 1,
                                         .rtvFormats = { RESOURCE_FORMAT_HIGHLIGHT_SELECT },
                                         .dsvFormat = PixelFormat::D24_UNORM_S8_UINT,  // gbuffer
                                     });

    ok = ok && Create(PSO_TONE, {
                                    .vs = "screenspace_quad.vs",
                                    .ps = "tone.ps",
                                    .rasterizerDesc = &s_rasterizerFrontFace,
                                    .depthStencilDesc = &s_depthStencilDefault,
                                    .inputLayoutDesc = &s_inputLayoutPosition,
                                    .numRenderTargets = 1,
                                    .rtvFormats = { RESOURCE_FORMAT_TONE },
                                    .dsvFormat = PixelFormat::D24_UNORM_S8_UINT,  // gbuffer
                                });

    // Bloom
    ok = ok && Create(PSO_BLOOM_SETUP, { .type = PipelineStateType::COMPUTE, .cs = "bloom_setup.cs" });
    ok = ok && Create(PSO_BLOOM_DOWNSAMPLE, { .type = PipelineStateType::COMPUTE, .cs = "bloom_downsample.cs" });
    ok = ok && Create(PSO_BLOOM_UPSAMPLE, { .type = PipelineStateType::COMPUTE, .cs = "bloom_upsample.cs" });

    // @HACK: only support this many shaders
    if (GraphicsManager::GetSingleton().GetBackend() == Backend::D3D12) {
        return ok;
    }

    // Particle
    ok = ok && Create(PSO_PARTICLE_INIT, { .type = PipelineStateType::COMPUTE, .cs = "particle_initialization.cs" });
    ok = ok && Create(PSO_PARTICLE_KICKOFF, { .type = PipelineStateType::COMPUTE, .cs = "particle_kickoff.cs" });
    ok = ok && Create(PSO_PARTICLE_EMIT, { .type = PipelineStateType::COMPUTE, .cs = "particle_emission.cs" });
    ok = ok && Create(PSO_PARTICLE_SIM, { .type = PipelineStateType::COMPUTE, .cs = "particle_simulation.cs" });
    ok = ok && Create(PSO_PARTICLE_RENDERING, {
                                                  .vs = "particle_draw.vs",
                                                  .ps = "particle_draw.ps",
                                                  .rasterizerDesc = &s_rasterizerFrontFace,
                                                  .depthStencilDesc = &s_depthStencilDefault,
                                                  .inputLayoutDesc = &s_inputLayoutMesh,
                                              });

    // @HACK: only support this many shaders
    if (GraphicsManager::GetSingleton().GetBackend() == Backend::D3D11) {
        return ok;
    }

    // Voxel
    ok = ok && Create(PSO_VOXELIZATION, {
                                            .vs = "voxelization.vs",
                                            .ps = "voxelization.ps",
                                            .gs = "voxelization.gs",
                                            .rasterizerDesc = &s_rasterizerDoubleSided,
                                            .depthStencilDesc = &s_depthStencilNoTest,
                                        });
    ok = ok && Create(PSO_VOXELIZATION_POST, { .type = PipelineStateType::COMPUTE, .cs = "post.cs" });
    ok = ok && Create(PSO_DEBUG_VOXEL, {
                                           .vs = "visualization.vs",
                                           .ps = "visualization.ps",
                                           .rasterizerDesc = &s_rasterizerFrontFace,
                                           .depthStencilDesc = &s_depthStencilDefault,
                                       });

    // PBR
    ok = ok && Create(PSO_ENV_SKYBOX_TO_CUBE_MAP, {
                                                      .vs = "cube_map.vs",
                                                      .ps = "to_cube_map.ps",
                                                      .rasterizerDesc = &s_rasterizerFrontFace,
                                                      .depthStencilDesc = &s_depthStencilDefault,
                                                  });
    ok = ok && Create(PSO_DIFFUSE_IRRADIANCE, {
                                                  .vs = "cube_map.vs",
                                                  .ps = "diffuse_irradiance.ps",
                                                  .rasterizerDesc = &s_rasterizerFrontFace,
                                                  .depthStencilDesc = &s_depthStencilDefault,
                                              });
    ok = ok && Create(PSO_PREFILTER, {
                                         .vs = "cube_map.vs",
                                         .ps = "prefilter.ps",
                                         .rasterizerDesc = &s_rasterizerFrontFace,
                                         .depthStencilDesc = &s_depthStencilDefault,
                                     });
    ok = ok && Create(PSO_ENV_SKYBOX, {
                                          .vs = "skybox.vs",
                                          .ps = "skybox.ps",
                                          .rasterizerDesc = &s_rasterizerFrontFace,
                                          .depthStencilDesc = &s_depthStencilDefault,
                                      });
    ok = ok && Create(PSO_BRDF, {
                                    .vs = "screenspace_quad.vs",
                                    .ps = "brdf.ps",
                                    .rasterizerDesc = &s_rasterizerFrontFace,
                                    .depthStencilDesc = &s_depthStencilNoTest,
                                });
    ok = ok && Create(PSO_IMAGE_2D, {
                                        .vs = "debug_draw_texture.vs",
                                        .ps = "debug_draw_texture.ps",
                                        .rasterizerDesc = &s_rasterizerFrontFace,
                                        .depthStencilDesc = &s_depthStencilNoTest,
                                    });
    ok = ok && Create(PSO_BILLBOARD, {
                                         .vs = "billboard.vs",
                                         .ps = "texture.ps",
                                         .rasterizerDesc = &s_rasterizerDoubleSided,
                                         .depthStencilDesc = &s_depthStencilDefault,
                                     });

    return ok;
}

void PipelineStateManager::Finalize() {
    for (size_t idx = 0; idx < m_cache.size(); ++idx) {
        m_cache[idx].reset();
    }
}

}  // namespace my
