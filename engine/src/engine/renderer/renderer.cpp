#include "renderer.h"

#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/renderer/render_data.h"

namespace my::renderer {

static RenderData* s_renderData;

void BeginFrame() {
    // @TODO: should be a better way
    if (s_renderData) {
        delete s_renderData;
        s_renderData = nullptr;
    }
}

void EndFrame() {
}

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene) {
    s_renderData = new RenderData();
    s_renderData->bakeIbl = false;
    RenderDataConfig config(p_scene);
    config.isOpengl = GraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;
    PrepareRenderData(p_camera, config, *s_renderData);
}

void RequestBakingIbl() {
    if (DEV_VERIFY(s_renderData)) {
        s_renderData->bakeIbl = true;
    }
}

const RenderData* GetRenderData() {
    return s_renderData;
}

}  // namespace my::renderer
