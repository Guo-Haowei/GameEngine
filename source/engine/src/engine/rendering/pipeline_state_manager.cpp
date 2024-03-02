#include "pipeline_state_manager.h"

#include "core/framework/asset_manager.h"
#include "core/framework/graphics_manager.h"

namespace my {

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

PipelineState* PipelineStateManager::find(PipelineStateName p_name) {
    DEV_ASSERT_INDEX(p_name, m_cache.size());
    return m_cache[p_name].get();
}

bool PipelineStateManager::initialize() {
    constexpr ShaderMacro has_animation = { "HAS_ANIMATION", "1" };
    {
        PipelineCreateInfo info;
        info.vs = "mesh.vert";
        info.ps = "gbuffer.frag";
        info.input_layout_desc = &s_input_layout_mesh;
        m_cache[PROGRAM_GBUFFER_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "mesh.vert";
        info.ps = "gbuffer.frag";
        info.defines = { has_animation };
        info.input_layout_desc = &s_input_layout_mesh;
        m_cache[PROGRAM_GBUFFER_ANIMATED] = create(info);
    }

    // @HACK: only support this many shaders
    if (GraphicsManager::singleton().get_backend() == Backend::D3D11) {
        return true;
    }

    {
        PipelineCreateInfo info;
        info.vs = "mesh.vert";
        info.ps = "base_color.frag";
        m_cache[PROGRAM_BASE_COLOR_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "mesh.vert";
        info.ps = "base_color.frag";
        info.defines = { has_animation };
        m_cache[PROGRAM_BASE_COLOR_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "shadow.vert";
        info.ps = "depth.frag";
        m_cache[PROGRAM_DPETH_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "shadow.vert";
        info.ps = "depth.frag";
        info.defines = { has_animation };
        m_cache[PROGRAM_DPETH_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "point_shadow.vert";
        info.ps = "point_shadow.frag";
        m_cache[PROGRAM_POINT_SHADOW_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "point_shadow.vert";
        info.ps = "point_shadow.frag";
        info.defines = { has_animation };
        m_cache[PROGRAM_POINT_SHADOW_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "ssao.frag";
        m_cache[PROGRAM_SSAO] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "lighting.frag";
        m_cache[PROGRAM_LIGHTING_VXGI] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "bloom_setup.comp";
        m_cache[PROGRAM_BLOOM_SETUP] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "bloom_downsample.comp";
        m_cache[PROGRAM_BLOOM_DOWNSAMPLE] = create(info);
    }
    //{
    //    PipelineCreateInfo info;
    //    info.cs = "bloom_upsample.comp";
    //    m_cache[PROGRAM_BLOOM_UPSAMPLE] = create(info);
    //}
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "tone.frag";
        m_cache[PROGRAM_TONE] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "voxelization.vert";
        info.gs = "voxelization.geom";
        info.ps = "voxelization.frag";
        m_cache[PROGRAM_VOXELIZATION_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "voxelization.vert";
        info.gs = "voxelization.geom";
        info.ps = "voxelization.frag";
        info.defines = { has_animation };
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
        info.ps = "visualization.frag";
        m_cache[PROGRAM_DEBUG_VOXEL] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "cube_map.vert";
        info.ps = "to_cube_map.frag";
        m_cache[PROGRAM_ENV_SKYBOX_TO_CUBE_MAP] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "cube_map.vert";
        info.ps = "diffuse_irradiance.frag";
        m_cache[PROGRAM_DIFFUSE_IRRADIANCE] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "cube_map.vert";
        info.ps = "prefilter.frag";
        m_cache[PROGRAM_PREFILTER] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "skybox.vert";
        info.ps = "skybox.frag";
        m_cache[PROGRAM_ENV_SKYBOX] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "screenspace_quad.vert";
        info.ps = "brdf.frag";
        m_cache[PROGRAM_BRDF] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "billboard.vert";
        info.ps = "texture.frag";
        m_cache[PROGRAM_BILLBOARD] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "debug_draw_texture.vert";
        info.ps = "debug_draw_texture.frag";
        m_cache[PROGRAM_IMAGE_2D] = create(info);
    }

    return true;
}

void PipelineStateManager::finalize() {
    for (size_t idx = 0; idx < m_cache.size(); ++idx) {
        m_cache[idx].reset();
    }
}

}  // namespace my
