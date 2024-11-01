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

// rasterizer states
static const RasterizerDesc s_default_rasterizer = {
    .fill_mode = FillMode::SOLID,
    .cull_mode = CullMode::BACK,
    .front_counter_clockwise = true,
};

static const RasterizerDesc s_shadow_rasterizer = {
    .fill_mode = FillMode::SOLID,
    .cull_mode = CullMode::FRONT,
    .front_counter_clockwise = true,
};

static const RasterizerDesc s_cull_none_rasterizer = {
    .fill_mode = FillMode::SOLID,
    .cull_mode = CullMode::NONE,
    .front_counter_clockwise = true,
};

static const DepthStencilDesc s_default_depth_stencil = {
    .depth_func = ComparisonFunc::LESS_EQUAL,
    .depth_enabled = true,
    .stencil_enabled = false,
};

static const DepthStencilDesc s_highlight_depth_stencil = {
    .depth_func = ComparisonFunc::LESS_EQUAL,
    .depth_enabled = false,
    .stencil_enabled = true,
    .op = DepthStencilOpDesc::EQUAL,
};

static const DepthStencilDesc s_gbuffer_depth_stencil = {
    .depth_func = ComparisonFunc::LESS_EQUAL,
    .depth_enabled = true,
    .stencil_enabled = true,
    .op = DepthStencilOpDesc::Z_PASS,
};

static const DepthStencilDesc s_no_depth_test = {
    .depth_func = ComparisonFunc::LESS_EQUAL,
    .depth_enabled = false,
    .stencil_enabled = false,
};

PipelineState* PipelineStateManager::Find(PipelineStateName p_name) {
    DEV_ASSERT_INDEX(p_name, m_cache.size());
    return m_cache[p_name].get();
}

bool PipelineStateManager::Create(PipelineStateName p_name, const PipelineCreateInfo& p_info) {
    ERR_FAIL_COND_V_MSG(m_cache[p_name] != nullptr, false, "pipeline already exists");

    auto pipeline = CreateInternal(p_info);
    ERR_FAIL_COND_V_MSG(pipeline == nullptr, false, std::format("failed to create pipeline ''"));

    m_cache[p_name] = pipeline;
    return true;
}

bool PipelineStateManager::Initialize() {
    constexpr ShaderMacro has_animation = { "HAS_ANIMATION", "1" };
    bool ok = true;

    {
        PipelineCreateInfo info;
        info.vs = "mesh.vert";
        info.ps = "gbuffer.pixel";
        info.input_layout_desc = &s_input_layout_mesh;
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_gbuffer_depth_stencil;
        ok = ok && Create(PROGRAM_GBUFFER_STATIC, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "mesh.vert";
        info.ps = "gbuffer.pixel";
        info.defines = { has_animation };
        info.input_layout_desc = &s_input_layout_mesh;
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_gbuffer_depth_stencil;
        ok = ok && Create(PROGRAM_GBUFFER_ANIMATED, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "lighting.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_position;
        ok = ok && Create(PROGRAM_LIGHTING, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "shadow.vert";
        info.ps = "depth.pixel";
        info.rasterizer_desc = &s_shadow_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_mesh;
        ok = ok && Create(PROGRAM_DPETH_STATIC, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "shadow.vert";
        info.ps = "depth.pixel";
        info.defines = { has_animation };
        info.rasterizer_desc = &s_shadow_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_mesh;
        ok = ok && Create(PROGRAM_DPETH_ANIMATED, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "shadowmap_point.vert";
        info.ps = "shadowmap_point.pixel";
        info.rasterizer_desc = &s_shadow_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_mesh;
        ok = ok && Create(PROGRAM_POINT_SHADOW_STATIC, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "shadowmap_point.vert";
        info.ps = "shadowmap_point.pixel";
        info.defines = { has_animation };
        info.rasterizer_desc = &s_shadow_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_mesh;
        ok = ok && Create(PROGRAM_POINT_SHADOW_ANIMATED, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "tone.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_position;
        ok = ok && Create(PROGRAM_TONE, info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "bloom_setup.comp";
        ok = ok && Create(PROGRAM_BLOOM_SETUP, info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "bloom_downsample.comp";
        ok = ok && Create(PROGRAM_BLOOM_DOWNSAMPLE, info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "bloom_upsample.comp";
        ok = ok && Create(PROGRAM_BLOOM_UPSAMPLE, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "particle.vert";
        info.ps = "particle.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_mesh;
        ok = ok && Create(PROGRAM_PARTICLE_RENDERING, info);
    }

    // @HACK: only support this many shaders
    if (GraphicsManager::GetSingleton().GetBackend() == Backend::D3D11) {
        return true;
    }

    {
        PipelineCreateInfo info;
        info.cs = "particle_simulation.comp";
        ok = ok && Create(PROGRAM_PARTICLE_SIMULATION, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "highlight.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_highlight_depth_stencil;
        ok = ok && Create(PROGRAM_HIGHLIGHT, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "voxelization.vert";
        info.gs = "voxelization.geom";
        info.ps = "voxelization.pixel";
        info.rasterizer_desc = &s_cull_none_rasterizer;
        info.depth_stencil_desc = &s_no_depth_test;
        ok = ok && Create(PROGRAM_VOXELIZATION_STATIC, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "voxelization.vert";
        info.gs = "voxelization.geom";
        info.ps = "voxelization.pixel";
        info.defines = { has_animation };
        info.rasterizer_desc = &s_cull_none_rasterizer;
        info.depth_stencil_desc = &s_no_depth_test;
        ok = ok && Create(PROGRAM_VOXELIZATION_ANIMATED, info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "post.comp";
        ok = ok && Create(PROGRAM_VOXELIZATION_POST, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "visualization.vert";
        info.ps = "visualization.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        ok = ok && Create(PROGRAM_DEBUG_VOXEL, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "cube_map.vert";
        info.ps = "to_cube_map.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        ok = ok && Create(PROGRAM_ENV_SKYBOX_TO_CUBE_MAP, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "cube_map.vert";
        info.ps = "diffuse_irradiance.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        ok = ok && Create(PROGRAM_DIFFUSE_IRRADIANCE, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "cube_map.vert";
        info.ps = "prefilter.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        ok = ok && Create(PROGRAM_PREFILTER, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "skybox.vert";
        info.ps = "skybox.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        ok = ok && Create(PROGRAM_ENV_SKYBOX, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "brdf.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_no_depth_test;
        ok = ok && Create(PROGRAM_BRDF, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "debug_draw_texture.vert";
        info.ps = "debug_draw_texture.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_no_depth_test;
        ok = ok && Create(PROGRAM_IMAGE_2D, info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "billboard.vert";
        info.ps = "texture.pixel";
        info.rasterizer_desc = &s_cull_none_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        ok = ok && Create(PROGRAM_BILLBOARD, info);
    }

    return ok;
}

void PipelineStateManager::Finalize() {
    for (size_t idx = 0; idx < m_cache.size(); ++idx) {
        m_cache[idx].reset();
    }
}

}  // namespace my
