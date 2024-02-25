#include "graphics_manager.h"

#include "core/debugger/profiler.h"
#include "rendering/empty/empty_graphics_manager.h"
#include "rendering/opengl/opengl_graphics_manager.h"

namespace my {

void GraphicsManager::event_received(std::shared_ptr<Event> event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(event.get()); e) {
        const Scene& scene = *e->get_scene();
        on_scene_change(scene);
    }
}

std::shared_ptr<GraphicsManager> GraphicsManager::create() {
    return std::make_shared<OpenGLGraphicsManager>();
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
