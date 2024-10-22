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

PipelineState* PipelineStateManager::find(PipelineStateName p_name) {
    DEV_ASSERT_INDEX(p_name, m_cache.size());
    return m_cache[p_name].get();
}

bool PipelineStateManager::initialize() {
    constexpr ShaderMacro has_animation = { "HAS_ANIMATION", "1" };
    {
        PipelineCreateInfo info;
        info.vs = "mesh.vert";
        info.ps = "gbuffer.pixel";
        info.input_layout_desc = &s_input_layout_mesh;
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_gbuffer_depth_stencil;
        m_cache[PROGRAM_GBUFFER_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "mesh.vert";
        info.ps = "gbuffer.pixel";
        info.defines = { has_animation };
        info.input_layout_desc = &s_input_layout_mesh;
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_gbuffer_depth_stencil;
        m_cache[PROGRAM_GBUFFER_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "lighting.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_position;
        m_cache[PROGRAM_LIGHTING] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "shadow.vert";
        info.ps = "depth.pixel";
        info.rasterizer_desc = &s_shadow_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_mesh;
        m_cache[PROGRAM_DPETH_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "shadow.vert";
        info.ps = "depth.pixel";
        info.defines = { has_animation };
        info.rasterizer_desc = &s_shadow_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_mesh;
        m_cache[PROGRAM_DPETH_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "tone.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        info.input_layout_desc = &s_input_layout_position;
        m_cache[PROGRAM_TONE] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "bloom_setup.comp";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_BLOOM_SETUP] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "bloom_downsample.comp";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_BLOOM_DOWNSAMPLE] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "bloom_upsample.comp";
        m_cache[PROGRAM_BLOOM_UPSAMPLE] = create(info);
    }

    // @HACK: only support this many shaders
    if (GraphicsManager::singleton().getBackend() == Backend::D3D11) {
        return true;
    }
    {
        PipelineCreateInfo info;
        info.vs = "point_shadow.vert";
        info.ps = "point_shadow.pixel";
        info.rasterizer_desc = &s_shadow_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_POINT_SHADOW_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "point_shadow.vert";
        info.ps = "point_shadow.pixel";
        info.defines = { has_animation };
        info.rasterizer_desc = &s_shadow_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_POINT_SHADOW_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "highlight.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_highlight_depth_stencil;
        m_cache[PROGRAM_HIGHLIGHT] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "voxelization.vert";
        info.gs = "voxelization.geom";
        info.ps = "voxelization.pixel";
        info.rasterizer_desc = &s_cull_none_rasterizer;
        info.depth_stencil_desc = &s_no_depth_test;
        m_cache[PROGRAM_VOXELIZATION_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "voxelization.vert";
        info.gs = "voxelization.geom";
        info.ps = "voxelization.pixel";
        info.defines = { has_animation };
        info.rasterizer_desc = &s_cull_none_rasterizer;
        info.depth_stencil_desc = &s_no_depth_test;
        m_cache[PROGRAM_VOXELIZATION_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "post.comp";
        m_cache[PROGRAM_VOXELIZATION_POST] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "visualization.vert";
        info.ps = "visualization.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_DEBUG_VOXEL] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "cube_map.vert";
        info.ps = "to_cube_map.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_ENV_SKYBOX_TO_CUBE_MAP] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "cube_map.vert";
        info.ps = "diffuse_irradiance.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_DIFFUSE_IRRADIANCE] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "cube_map.vert";
        info.ps = "prefilter.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_PREFILTER] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "skybox.vert";
        info.ps = "skybox.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_ENV_SKYBOX] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "brdf.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_no_depth_test;
        m_cache[PROGRAM_BRDF] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "debug_draw_texture.vert";
        info.ps = "debug_draw_texture.pixel";
        info.rasterizer_desc = &s_default_rasterizer;
        info.depth_stencil_desc = &s_no_depth_test;
        m_cache[PROGRAM_IMAGE_2D] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "billboard.vert";
        info.ps = "texture.pixel";
        info.rasterizer_desc = &s_cull_none_rasterizer;
        info.depth_stencil_desc = &s_default_depth_stencil;
        m_cache[PROGRAM_BILLBOARD] = create(info);
    }

    return true;
}

void PipelineStateManager::finalize() {
    for (size_t idx = 0; idx < m_cache.size(); ++idx) {
        m_cache[idx].reset();
    }
}

}  // namespace my
