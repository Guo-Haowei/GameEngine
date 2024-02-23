#include "pipeline_state_manager.h"

#include <sstream>
#include <vector>

#include "core/framework/asset_manager.h"
#include "rendering/GLPrerequisites.h"

namespace my {

PipelineState* PipelineStateManager::find(PipelineStateName p_name) {
    DEV_ASSERT_INDEX(p_name, m_cache.size());
    return m_cache[p_name].get();
}

bool PipelineStateManager::initialize() {
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/mesh_static.vert.glsl";
        info.ps = "@res://glsl/base_color.frag.glsl";
        m_cache[PROGRAM_BASE_COLOR_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/mesh_animated.vert.glsl";
        info.ps = "@res://glsl/base_color.frag.glsl";
        m_cache[PROGRAM_BASE_COLOR_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/mesh_static.vert.glsl";
        info.ps = "@res://glsl/gbuffer.frag.glsl";
        m_cache[PROGRAM_GBUFFER_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/mesh_animated.vert.glsl";
        info.ps = "@res://glsl/gbuffer.frag.glsl";
        m_cache[PROGRAM_GBUFFER_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/shadow_static.vert.glsl";
        info.ps = "@res://glsl/depth.frag";
        m_cache[PROGRAM_DPETH_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/shadow_animated.vert.glsl";
        info.ps = "@res://glsl/depth.frag";
        m_cache[PROGRAM_DPETH_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/point_shadow_static.vert.glsl";
        info.gs = "@res://glsl/point_shadow.geom.glsl";
        info.ps = "@res://glsl/point_shadow.frag.glsl";
        m_cache[PROGRAM_POINT_SHADOW_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/point_shadow_animated.vert.glsl";
        info.gs = "@res://glsl/point_shadow.geom.glsl";
        info.ps = "@res://glsl/point_shadow.frag.glsl";
        m_cache[PROGRAM_POINT_SHADOW_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/fullscreen.vert.glsl";
        info.ps = "@res://glsl/ssao.frag";
        m_cache[PROGRAM_SSAO] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/fullscreen.vert.glsl";
        info.ps = "@res://glsl/lighting.frag.glsl";
        m_cache[PROGRAM_LIGHTING_VXGI] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/fullscreen.vert.glsl";
        info.ps = "@res://glsl/fxaa.frag";
        m_cache[PROGRAM_FXAA] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/vxgi/voxelization_static.vert.glsl";
        info.gs = "@res://glsl/vxgi/voxelization.geom.glsl";
        info.ps = "@res://glsl/vxgi/voxelization.frag.glsl";
        m_cache[PROGRAM_VOXELIZATION_STATIC] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/vxgi/voxelization_animated.vert.glsl";
        info.gs = "@res://glsl/vxgi/voxelization.geom.glsl";
        info.ps = "@res://glsl/vxgi/voxelization.frag.glsl";
        m_cache[PROGRAM_VOXELIZATION_ANIMATED] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.cs = "@res://glsl/vxgi/post.comp.glsl";
        m_cache[PROGRAM_VOXELIZATION_POST] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/vxgi/visualization.vert.glsl";
        info.ps = "@res://glsl/vxgi/visualization.frag.glsl";
        m_cache[PROGRAM_DEBUG_VOXEL] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/cube_map.vert.glsl";
        info.ps = "@res://glsl/cube_map.frag.glsl";
        m_cache[PROGRAM_SKY_BOX] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/billboard.vert.glsl";
        info.ps = "@res://glsl/texture.frag.glsl";
        m_cache[PROGRAM_BILLBOARD] = create(info);
    }
    {
        PipelineCreateInfo info;
        info.vs = "@res://glsl/debug_draw_texture.vert.glsl";
        info.ps = "@res://glsl/debug_draw_texture.frag.glsl";
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
