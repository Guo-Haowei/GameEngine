#include "pipeline_state_manager.h"

#include "core/framework/asset_manager.h"
#include "core/framework/graphics_manager.h"

namespace my {

// input layouts
static const InputLayoutDesc s_input_layout_mesh = {
    .elements = {
        { "POSITION", 0, PixelFormat::R32G32B32_FLOAT, 0, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, PixelFormat::R32G32B32_FLOAT, 1, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, PixelFormat::R32G32_FLOAT, 2, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "TANGENT", 0, PixelFormat::R32G32B32_FLOAT, 3, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "BONEINDEX", 0, PixelFormat::R32G32B32A32_SINT, 4, 0, InputClassification::PER_VERTEX_DATA, 0 },
        { "BONEWEIGHT", 0, PixelFormat::R32G32B32A32_FLOAT, 5, 0, InputClassification::PER_VERTEX_DATA, 0 },
    }
};

static const InputLayoutDesc s_input_layout_position = {
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
    .depthFunc = ComparisonFunc::LESS_EQUAL,
    .depthEnabled = true,
    .stencilEnabled = false,
};

static const DepthStencilDesc s_depthStencilHighlight = {
    .depthFunc = ComparisonFunc::LESS_EQUAL,
    .depthEnabled = false,
    .stencilEnabled = true,
    .op = DepthStencilOpDesc::EQUAL,
};

static const DepthStencilDesc s_depthStencilGbuffer = {
    .depthFunc = ComparisonFunc::LESS_EQUAL,
    .depthEnabled = true,
    .stencilEnabled = true,
    .op = DepthStencilOpDesc::Z_PASS,
};

static const DepthStencilDesc s_depthStencilNoTest = {
    .depthFunc = ComparisonFunc::LESS_EQUAL,
    .depthEnabled = false,
    .stencilEnabled = false,
};

PipelineState* PipelineStateManager::Find(PipelineStateName p_name) {
    DEV_ASSERT_INDEX(p_name, m_cache.size());
    return m_cache[p_name].get();
}

bool PipelineStateManager::Create(PipelineStateName p_name, const PipelineStateDesc& p_info) {
    ERR_FAIL_COND_V_MSG(m_cache[p_name] != nullptr, false, "pipeline already exists");

    auto pipeline = CreateInternal(p_info);
    ERR_FAIL_COND_V_MSG(pipeline == nullptr, false, std::format("failed to create pipeline ''"));

    m_cache[p_name] = pipeline;
    return true;
}

bool PipelineStateManager::Initialize() {
    bool ok = true;

    ok = ok && Create(
                   PROGRAM_GBUFFER,
                   {
                       .vs = "mesh.vert",
                       .ps = "gbuffer.pixel",
                       .rasterizerDesc = &s_rasterizerFrontFace,
                       .depthStencilDesc = &s_depthStencilGbuffer,
                       .inputLayoutDesc = &s_input_layout_mesh,
                       .numRenderTargets = 4,
                       .rtvFormats = { GBUFFER_BASE_COLOR_FORMAT, GBUFFER_POSITION_FORMAT, GBUFFER_NORMAL_FORMAT, GBUFFER_MATERIAL_FORMAT },
                       .dsvFormat = PixelFormat::D24_UNORM_S8_UINT,
                   });
    ok = ok && Create(PROGRAM_DPETH, {
                                         .vs = "shadow.vert",
                                         .ps = "depth.pixel",
                                         .rasterizerDesc = &s_rasterizerBackFace,
                                         .depthStencilDesc = &s_depthStencilDefault,
                                         .inputLayoutDesc = &s_input_layout_mesh,
                                         .numRenderTargets = 0,
                                         .dsvFormat = PixelFormat::D32_FLOAT,
                                     });
    ok = ok && Create(PROGRAM_POINT_SHADOW, {
                                                .vs = "shadowmap_point.vert",
                                                .ps = "shadowmap_point.pixel",
                                                .rasterizerDesc = &s_rasterizerBackFace,
                                                .depthStencilDesc = &s_depthStencilDefault,
                                                .inputLayoutDesc = &s_input_layout_mesh,
                                                .numRenderTargets = 0,
                                                .dsvFormat = PixelFormat::D32_FLOAT,
                                            });
    ok = ok && Create(PROGRAM_LIGHTING, {
                                            .vs = "screenspace_quad.vert",
                                            .ps = "lighting.pixel",
                                            .rasterizerDesc = &s_rasterizerFrontFace,
                                            .depthStencilDesc = &s_depthStencilDefault,
                                            .inputLayoutDesc = &s_input_layout_position,
                                        });

    // @HACK: only support this many shaders
    if (GraphicsManager::GetSingleton().GetBackend() == Backend::D3D12) {
        return ok;
    }
    ok = ok && Create(PROGRAM_TONE, {
                                        .vs = "screenspace_quad.vert",
                                        .ps = "tone.pixel",
                                        .rasterizerDesc = &s_rasterizerFrontFace,
                                        .depthStencilDesc = &s_depthStencilDefault,
                                        .inputLayoutDesc = &s_input_layout_position,
                                    });

    // Bloom
    ok = ok && Create(PROGRAM_BLOOM_SETUP, { .cs = "bloom_setup.comp" });
    ok = ok && Create(PROGRAM_BLOOM_DOWNSAMPLE, { .cs = "bloom_downsample.comp" });
    ok = ok && Create(PROGRAM_BLOOM_UPSAMPLE, { .cs = "bloom_upsample.comp" });

    // Particle
    ok = ok && Create(PROGRAM_PARTICLE_INIT, { .cs = "particle_initialization.comp" });
    ok = ok && Create(PROGRAM_PARTICLE_KICKOFF, { .cs = "particle_kickoff.comp" });
    ok = ok && Create(PROGRAM_PARTICLE_EMIT, { .cs = "particle_emission.comp" });
    ok = ok && Create(PROGRAM_PARTICLE_SIM, { .cs = "particle_simulation.comp" });
    ok = ok && Create(PROGRAM_PARTICLE_RENDERING, {
                                                      .vs = "particle_draw.vert",
                                                      .ps = "particle_draw.pixel",
                                                      .rasterizerDesc = &s_rasterizerFrontFace,
                                                      .depthStencilDesc = &s_depthStencilDefault,
                                                      .inputLayoutDesc = &s_input_layout_mesh,
                                                  });

    // @HACK: only support this many shaders
    if (GraphicsManager::GetSingleton().GetBackend() == Backend::D3D11) {
        return ok;
    }
    ok = ok && Create(PROGRAM_HIGHLIGHT, {
                                             .vs = "screenspace_quad.vert",
                                             .ps = "highlight.pixel",
                                             .rasterizerDesc = &s_rasterizerFrontFace,
                                             .depthStencilDesc = &s_depthStencilHighlight,
                                         });

    // Voxel
    ok = ok && Create(PROGRAM_VOXELIZATION, {
                                                .vs = "voxelization.vert",
                                                .ps = "voxelization.pixel",
                                                .gs = "voxelization.geom",
                                                .rasterizerDesc = &s_rasterizerDoubleSided,
                                                .depthStencilDesc = &s_depthStencilNoTest,
                                            });
    ok = ok && Create(PROGRAM_VOXELIZATION_POST, { .cs = "post.comp" });
    ok = ok && Create(PROGRAM_DEBUG_VOXEL, {
                                               .vs = "visualization.vert",
                                               .ps = "visualization.pixel",
                                               .rasterizerDesc = &s_rasterizerFrontFace,
                                               .depthStencilDesc = &s_depthStencilDefault,
                                           });

    // PBR
    ok = ok && Create(PROGRAM_ENV_SKYBOX_TO_CUBE_MAP, {
                                                          .vs = "cube_map.vert",
                                                          .ps = "to_cube_map.pixel",
                                                          .rasterizerDesc = &s_rasterizerFrontFace,
                                                          .depthStencilDesc = &s_depthStencilDefault,
                                                      });
    ok = ok && Create(PROGRAM_DIFFUSE_IRRADIANCE, {
                                                      .vs = "cube_map.vert",
                                                      .ps = "diffuse_irradiance.pixel",
                                                      .rasterizerDesc = &s_rasterizerFrontFace,
                                                      .depthStencilDesc = &s_depthStencilDefault,
                                                  });
    ok = ok && Create(PROGRAM_PREFILTER, {
                                             .vs = "cube_map.vert",
                                             .ps = "prefilter.pixel",
                                             .rasterizerDesc = &s_rasterizerFrontFace,
                                             .depthStencilDesc = &s_depthStencilDefault,
                                         });
    ok = ok && Create(PROGRAM_ENV_SKYBOX, {
                                              .vs = "skybox.vert",
                                              .ps = "skybox.pixel",
                                              .rasterizerDesc = &s_rasterizerFrontFace,
                                              .depthStencilDesc = &s_depthStencilDefault,
                                          });
    ok = ok && Create(PROGRAM_BRDF, {
                                        .vs = "screenspace_quad.vert",
                                        .ps = "brdf.pixel",
                                        .rasterizerDesc = &s_rasterizerFrontFace,
                                        .depthStencilDesc = &s_depthStencilNoTest,
                                    });
    ok = ok && Create(PROGRAM_IMAGE_2D, {
                                            .vs = "debug_draw_texture.vert",
                                            .ps = "debug_draw_texture.pixel",
                                            .rasterizerDesc = &s_rasterizerFrontFace,
                                            .depthStencilDesc = &s_depthStencilNoTest,
                                        });
    ok = ok && Create(PROGRAM_BILLBOARD, {
                                             .vs = "billboard.vert",
                                             .ps = "texture.pixel",
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
