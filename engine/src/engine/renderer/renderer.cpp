#include "renderer.h"

#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/renderer/render_data.h"

#define DEFINE_DVAR
#include "graphics_dvars.h"

namespace my::renderer {

static RenderData* s_renderData;
std::list<PointShadowHandle> s_freePointLightShadows = { 0, 1, 2, 3, 4, 5, 6, 7 };

void RegisterDvars() {
#define REGISTER_DVAR
#include "graphics_dvars.h"
}

void BeginFrame() {
    // @TODO: should be a better way
    if (s_renderData) {
        delete s_renderData;
        s_renderData = nullptr;
    }
    s_renderData = new RenderData();
    s_renderData->bakeIbl = false;
}

void EndFrame() {
}

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene) {
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

PointShadowHandle AllocatePointLightShadowMap() {
    if (s_freePointLightShadows.empty()) {
        LOG_WARN("OUT OUT POINT SHADOW MAP");
        return INVALID_POINT_SHADOW_HANDLE;
    }

    int handle = s_freePointLightShadows.front();
    s_freePointLightShadows.pop_front();
    return handle;
}

void FreePointLightShadowMap(PointShadowHandle& p_handle) {
    DEV_ASSERT_INDEX(p_handle, MAX_POINT_LIGHT_SHADOW_COUNT);
    s_freePointLightShadows.push_back(p_handle);
    p_handle = INVALID_POINT_SHADOW_HANDLE;
}

}  // namespace my::renderer
