#include "graphics_manager.h"

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

}  // namespace my
