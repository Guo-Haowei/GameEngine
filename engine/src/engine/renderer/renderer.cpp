#include "renderer.h"

#include "engine/core/framework/application.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/renderer/render_data.h"

namespace my::renderer {

static RenderData* g_renderData;

void BeginFrame(Application* p_app) {
    // @TODO: should be a better way
    if (g_renderData) {
        delete g_renderData;
    }

    Scene* scene = p_app->GetSceneManager()->GetScenePtr();
    if (DEV_VERIFY(scene)) {
        g_renderData = new RenderData();
        RenderDataConfig config(*scene);
        config.isOpengl = p_app->GetGraphicsManager()->GetBackend() == Backend::OPENGL;
        PrepareRenderData(*scene->m_camera.get(), config, *g_renderData);
    }
}

void EndFrame() {
}

const RenderData* GetRenderData() {
    return g_renderData;
}

}  // namespace my::renderer
