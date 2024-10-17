#include "graphics_manager.h"

#include "core/debugger/profiler.h"
#include "drivers/d3d11/d3d11_graphics_manager.h"
#include "drivers/empty/empty_graphics_manager.h"
#include "drivers/opengl/opengl_graphics_manager.h"
#include "rendering/render_graph/render_graph_defines.h"
#include "rendering/render_manager.h"
#include "rendering/rendering_dvars.h"

namespace my {

bool GraphicsManager::initialize() {
    if (!initializeImpl()) {
        return false;
    }

    m_render_data = std::make_shared<RenderData>();

    DEV_ASSERT(m_pipeline_state_manager);

    if (!m_pipeline_state_manager->initialize()) {
        return false;
    }

    auto bind_slot = [&](const std::string& name, int slot, Dimension p_dimension = Dimension::TEXTURE_2D) {
        std::shared_ptr<RenderTarget> resource = findRenderTarget(name);
        if (!resource) {
            return;
        }

        bindTexture(p_dimension, resource->texture->get_handle(), slot);
    };

    // bind common textures
    bind_slot(RT_RES_HIGHLIGHT_SELECT, u_selection_highlight_slot);
    bind_slot(RT_RES_GBUFFER_BASE_COLOR, u_gbuffer_base_color_map_slot);
    bind_slot(RT_RES_GBUFFER_POSITION, u_gbuffer_position_map_slot);
    bind_slot(RT_RES_GBUFFER_NORMAL, u_gbuffer_normal_map_slot);
    bind_slot(RT_RES_GBUFFER_MATERIAL, u_gbuffer_material_map_slot);

    return true;
}

void GraphicsManager::eventReceived(std::shared_ptr<Event> event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(event.get()); e) {
        const Scene& scene = *e->getScene();
        onSceneChange(scene);
    }
    if (ResizeEvent* e = dynamic_cast<ResizeEvent*>(event.get()); e) {
        onWindowResize(e->getWidth(), e->getHeight());
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

void GraphicsManager::setPipelineState(PipelineStateName p_name) {
    if (m_last_pipeline_name != p_name) {
        setPipelineStateImpl(p_name);
        m_last_pipeline_name = p_name;
    }
}

void GraphicsManager::requestTexture(ImageHandle* p_handle, OnTextureLoadFunc p_func) {
    m_loaded_images.push(ImageTask{ p_handle, p_func });
}

void GraphicsManager::update(Scene&) {
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

        image->gpu_texture = createTexture(texture_desc, sampler_desc);
        if (task.func) {
            task.func(task.handle->get());
        }
    }

    render();
}

void GraphicsManager::selectRenderGraph() {
    std::string method(DVAR_GET_STRING(r_render_graph));
    if (method == "vxgi") {
        m_method = RenderGraph::VXGI;
    } else {
        m_method = RenderGraph::DEFAULT;
    }

    if (m_backend == Backend::D3D11) {
        m_method = RenderGraph::DEFAULT;
    }

    switch (m_method) {
        case RenderGraph::VXGI:
            createRenderGraphVxgi(m_render_graph);
            break;
        default:
            createRenderGraphDefault(m_render_graph);
            break;
    }
}

std::shared_ptr<RenderTarget> GraphicsManager::createRenderTarget(const RenderTargetDesc& p_desc, const SamplerDesc& p_sampler) {
    DEV_ASSERT(m_resource_lookup.find(p_desc.name) == m_resource_lookup.end());
    std::shared_ptr<RenderTarget> resource = std::make_shared<RenderTarget>(p_desc);

    // @TODO: this part need rework
    TextureDesc texture_desc{};
    switch (p_desc.type) {
        case AttachmentType::COLOR_2D:
        case AttachmentType::DEPTH_2D:
        case AttachmentType::DEPTH_STENCIL_2D:
        case AttachmentType::SHADOW_2D:
            texture_desc.dimension = Dimension::TEXTURE_2D;
            break;
        case AttachmentType::SHADOW_CUBE_MAP:
        case AttachmentType::COLOR_CUBE_MAP:
            texture_desc.dimension = Dimension::TEXTURE_CUBE;
            break;
        default:
            CRASH_NOW();
            break;
    }
    switch (p_desc.type) {
        case AttachmentType::COLOR_2D:
        case AttachmentType::COLOR_CUBE_MAP:
            texture_desc.bind_flags |= BIND_SHADER_RESOURCE | BIND_RENDER_TARGET;
            break;
        case AttachmentType::SHADOW_2D:
        case AttachmentType::SHADOW_CUBE_MAP:
            texture_desc.bind_flags |= BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
            break;
        case AttachmentType::DEPTH_2D:
        case AttachmentType::DEPTH_STENCIL_2D:
            texture_desc.bind_flags |= BIND_DEPTH_STENCIL;
            break;
        default:
            break;
    }

    texture_desc.format = p_desc.format;
    texture_desc.width = p_desc.width;
    texture_desc.height = p_desc.height;
    texture_desc.initial_data = nullptr;
    texture_desc.misc_flags = 0;
    texture_desc.mip_levels = 1;
    texture_desc.array_size = 1;
    if (p_desc.gen_mipmap) {
        texture_desc.misc_flags |= RESOURCE_MISC_GENERATE_MIPS;
    }

    resource->texture = createTexture(texture_desc, p_sampler);

    m_resource_lookup[resource->desc.name] = resource;
    return resource;
}

std::shared_ptr<RenderTarget> GraphicsManager::findRenderTarget(const std::string& name) const {
    if (m_resource_lookup.empty()) {
        return nullptr;
    }

    auto it = m_resource_lookup.find(name);
    if (it == m_resource_lookup.end()) {
        return nullptr;
    }
    return it->second;
}

uint64_t GraphicsManager::getFinalImage() const {
    const Texture* texture = nullptr;
    switch (m_method) {
        case RenderGraph::VXGI:
            texture = findRenderTarget(RT_RES_FINAL)->texture.get();
            break;
        case RenderGraph::DEFAULT:
            texture = findRenderTarget(RT_RES_LIGHTING)->texture.get();
            break;
        default:
            CRASH_NOW();
            break;
    }

    if (texture) {
        return texture->get_handle();
    }

    return 0;
}

}  // namespace my
