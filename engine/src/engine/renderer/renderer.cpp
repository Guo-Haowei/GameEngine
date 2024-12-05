#include "renderer.h"

#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/renderer/render_data.h"

namespace my::renderer {

static RenderData* g_renderData;

void BeginFrame() {
    // @TODO: should be a better way
    if (g_renderData) {
        delete g_renderData;
        g_renderData = nullptr;
    }
}

void EndFrame() {
}

void RequestScene(const CameraComponent& p_camera, Scene& p_scene) {
    g_renderData = new RenderData();
    RenderDataConfig config(p_scene);
    config.isOpengl = GraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;
    PrepareRenderData(p_camera, config, *g_renderData);
}

const RenderData* GetRenderData() {
    return g_renderData;
}

}  // namespace my::renderer
