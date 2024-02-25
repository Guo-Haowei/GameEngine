#include "graphics_manager.h"

#include "core/debugger/profiler.h"
#include "drivers/d3d11/d3d11_graphics_manager.h"
#include "drivers/empty/empty_graphics_manager.h"
#include "drivers/opengl/opengl_graphics_manager.h"
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
    return std::make_shared<EmptyGraphicsManager>();
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
        create_texture(task.handle);
        if (task.func) {
            task.func(task.handle);
        }
    }

    render();
}

}  // namespace my
