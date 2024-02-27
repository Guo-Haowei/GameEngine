#include "graphics_manager.h"

#include "core/debugger/profiler.h"
#include "drivers/d3d11/d3d11_graphics_manager.h"
#include "drivers/empty/empty_graphics_manager.h"
#include "drivers/opengl/opengl_graphics_manager.h"
#include "rendering/render_graph/render_graphs.h"
#include "rendering/renderer.h"
#include "rendering/rendering_dvars.h"

namespace my {

void GraphicsManager::event_received(std::shared_ptr<Event> event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(event.get()); e) {
        const Scene& scene = *e->get_scene();
        on_scene_change(scene);
    }
    if (ResizeEvent* e = dynamic_cast<ResizeEvent*>(event.get()); e) {
        on_window_resize(e->get_width(), e->get_height());
    }
}

std::shared_ptr<GraphicsManager> GraphicsManager::create() {
    const std::string& backend = DVAR_GET_STRING(r_backend);

    if (backend == "opengl") {
        return std::make_shared<OpenGLGraphicsManager>();
    } else if (backend == "d3d11") {
        return std::make_shared<D3d11GraphicsManager>();
    }
    return std::make_shared<EmptyGraphicsManager>(Backend::EMPTY);
}

void GraphicsManager::set_pipeline_state(PipelineStateName p_name) {
    if (m_last_pipeline_name != p_name) {
        set_pipeline_state_impl(p_name);
        m_last_pipeline_name = p_name;
    }
}

void GraphicsManager::request_texture(ImageHandle* p_handle, OnTextureLoadFunc p_func) {
    m_loaded_images.push(ImageTask{ p_handle, p_func });
}

void GraphicsManager::update(float) {
    OPTICK_EVENT();

    auto loaded_images = m_loaded_images.pop_all();
    while (!loaded_images.empty()) {
        auto task = loaded_images.front();
        loaded_images.pop();
        DEV_ASSERT(task.handle->state == ASSET_STATE_READY);
        Image* image = task.handle->get();
        DEV_ASSERT(image);

        TextureDesc texture_desc{};
        SamplerDesc sampler_desc{};
        renderer::fill_texture_and_sampler_desc(image, texture_desc, sampler_desc);

        image->gpu_texture = create_texture(texture_desc, sampler_desc);
        if (task.func) {
            task.func(task.handle->get());
        }
    }

    render();
}

void GraphicsManager::select_render_graph() {
    std::string method(DVAR_GET_STRING(r_render_graph));
    if (method == "vxgi") {
        m_method = RenderGraph::VXGI;
    } else if (method == "base_color") {
        m_method = RenderGraph::BASE_COLOR;
    } else {
        m_method = RenderGraph::DUMMY;
    }

    if (m_backend == Backend::D3D11) {
        m_method = RenderGraph::DUMMY;
    }

    switch (m_method) {
        case RenderGraph::BASE_COLOR:
            create_render_graph_base_color(m_render_graph);
            break;
        case RenderGraph::VXGI:
            create_render_graph_vxgi(m_render_graph);
            break;
        default:
            create_render_graph_dummy(m_render_graph);
            break;
    }
}

std::shared_ptr<RenderTarget> GraphicsManager::create_render_target(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler) {
    DEV_ASSERT(m_resource_lookup.find(p_desc.name) == m_resource_lookup.end());
    std::shared_ptr<RenderTarget> resource = std::make_shared<RenderTarget>(p_desc);

    TextureDesc texture_desc{};
    switch (p_desc.type) {
        case AttachmentType::COLOR_2D:
        case AttachmentType::DEPTH_2D:
        case AttachmentType::SHADOW_2D:
            texture_desc.dimension = Dimension::TEXTURE_2D;
            break;
        case AttachmentType::SHADOW_CUBE_MAP:
        case AttachmentType::COLOR_CUBE_MAP:
            texture_desc.dimension = Dimension::TEXTURE_CUBE;
            break;
        default:
            break;
    }
    texture_desc.format = p_desc.format;
    texture_desc.width = p_desc.width;
    texture_desc.height = p_desc.height;
    texture_desc.initial_data = nullptr;
    texture_desc.misc_flags = 0;
    texture_desc.bind_flags |= BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
    texture_desc.mip_levels = 1;
    texture_desc.array_size = 1;
    if (p_desc.gen_mipmap) {
        texture_desc.misc_flags |= RESOURCE_MISC_GENERATE_MIPS;
    }

    resource->texture = create_texture(texture_desc, p_sampler);

    m_resource_lookup[resource->desc.name] = resource;
    return resource;
}

std::shared_ptr<RenderTarget> GraphicsManager::find_render_target(const std::string& name) const {
    if (m_resource_lookup.empty()) {
        return nullptr;
    }

    auto it = m_resource_lookup.find(name);
    if (it == m_resource_lookup.end()) {
        return nullptr;
    }
    return it->second;
}

uint64_t GraphicsManager::get_final_image() const {
    const Texture* texture = nullptr;
    switch (m_method) {
        case RenderGraph::VXGI:
            texture = find_render_target(RT_RES_FINAL)->texture.get();
            break;
        case RenderGraph::BASE_COLOR:
            return find_render_target(RT_RES_BASE_COLOR)->texture->get_handle();
        case RenderGraph::DUMMY:
            texture = find_render_target(RT_RES_FINAL)->texture.get();
            break;
        default:
            CRASH_NOW();
            break;
    }

    if (texture) {
        return texture->get_imgui_handle();
    }

    return 0;
}

}  // namespace my
