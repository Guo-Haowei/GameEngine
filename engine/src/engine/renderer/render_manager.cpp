#include "render_manager.h"

#include "engine/core/debugger/profiler.h"
#include "engine/core/framework/asset_manager.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/math/geometry.h"
#include "engine/renderer/render_graph/render_graph_defines.h"
#include "engine/scene/scene.h"

#define DEFINE_DVAR
#include "graphics_dvars.h"

namespace {
std::string s_prev_env_map;
}  // namespace

namespace my::renderer {

// @TODO: fix this?
void register_rendering_dvars() {
#define REGISTER_DVAR
#include "graphics_dvars.h"
}

void fill_texture_and_sampler_desc(const ImageAsset* p_image, GpuTextureDesc& p_texture_desc, SamplerDesc& p_sampler_desc) {
    DEV_ASSERT(p_image);
    bool is_hdr_file = false;

    switch (p_image->format) {
        case PixelFormat::R32_FLOAT:
        case PixelFormat::R32G32_FLOAT:
        case PixelFormat::R32G32B32_FLOAT:
        case PixelFormat::R32G32B32A32_FLOAT: {
            is_hdr_file = true;
        } break;
        default: {
        } break;
    }

    p_texture_desc.format = p_image->format;
    p_texture_desc.dimension = Dimension::TEXTURE_2D;
    p_texture_desc.width = p_image->width;
    p_texture_desc.height = p_image->height;
    p_texture_desc.arraySize = 1;
    p_texture_desc.bindFlags |= BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    p_texture_desc.initialData = p_image->buffer.data();
    p_texture_desc.mipLevels = 1;

    if (is_hdr_file) {
        p_sampler_desc.minFilter = p_sampler_desc.magFilter = FilterMode::LINEAR;
        p_sampler_desc.addressU = p_sampler_desc.addressV = AddressMode::CLAMP;
        p_texture_desc.bindFlags &= (~BIND_RENDER_TARGET);
    } else {
        p_texture_desc.miscFlags |= RESOURCE_MISC_GENERATE_MIPS;
        p_sampler_desc.minFilter = FilterMode::MIPMAP_LINEAR;
        p_sampler_desc.magFilter = FilterMode::LINEAR;
    }
}

}  // namespace my::renderer

namespace my {

RenderManager::RenderManager() : Module("RenderManager") {
    for (int i = 0; i < MAX_POINT_LIGHT_SHADOW_COUNT; ++i) {
        m_free_point_light_shadow.push_back(i);
    }
}

auto RenderManager::InitializeImpl() -> Result<void> {
    m_screen_quad_buffers = GraphicsManager::GetSingleton().CreateMesh(MakePlaneMesh(Vector3f(1)));
    m_skybox_buffers = GraphicsManager::GetSingleton().CreateMesh(MakeSkyBoxMesh());

    return Result<void>();
}

void RenderManager::FinalizeImpl() {
    m_screen_quad_buffers = nullptr;
    m_skybox_buffers = nullptr;

    m_free_point_light_shadow.clear();
    return;
}

PointShadowHandle RenderManager::allocate_point_light_shadowMap() {
    if (m_free_point_light_shadow.empty()) {
        LOG_WARN("OUT OUT POINT SHADOW MAP");
        return INVALID_POINT_SHADOW_HANDLE;
    }

    int handle = m_free_point_light_shadow.front();
    m_free_point_light_shadow.pop_front();
    return handle;
}

void RenderManager::free_point_light_shadowMap(PointShadowHandle& p_handle) {
    DEV_ASSERT_INDEX(p_handle, MAX_POINT_LIGHT_SHADOW_COUNT);
    m_free_point_light_shadow.push_back(p_handle);
    p_handle = INVALID_POINT_SHADOW_HANDLE;
}

void RenderManager::draw_quad() {
    GraphicsManager::GetSingleton().SetMesh(m_screen_quad_buffers);
    GraphicsManager::GetSingleton().DrawElements(m_screen_quad_buffers->indexCount);
}

void RenderManager::draw_quad_instanced(uint32_t p_instance_count) {
    GraphicsManager::GetSingleton().SetMesh(m_screen_quad_buffers);
    GraphicsManager::GetSingleton().DrawElementsInstanced(p_instance_count, m_screen_quad_buffers->indexCount, 0);
}

void RenderManager::draw_skybox() {
    GraphicsManager::GetSingleton().SetMesh(m_skybox_buffers);
    GraphicsManager::GetSingleton().DrawElements(m_skybox_buffers->indexCount);
}

}  // namespace my
