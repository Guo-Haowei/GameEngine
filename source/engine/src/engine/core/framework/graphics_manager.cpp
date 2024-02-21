#include "graphics_manager.h"

#include "rendering/empty/empty_graphics_manager.h"
#include "rendering/opengl/gl_graphics_manager.h"

namespace my {

void GraphicsManager::event_received(std::shared_ptr<Event> event) {
    if (SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(event.get()); e) {
        const Scene& scene = *e->get_scene();
        on_scene_change(scene);
    }
}

std::shared_ptr<GraphicsManager> GraphicsManager::create() {
    return std::make_shared<EmptyGraphicsManager>();
    // return std::make_shared<GLGraphicsManager>();
}

}  // namespace my
