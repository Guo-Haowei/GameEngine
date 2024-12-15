#include "renderer.h"

#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/renderer/draw_data.h"

#define DEFINE_DVAR
#include "graphics_dvars.h"

namespace my::renderer {

static DrawData* s_renderData;
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
    s_renderData = new DrawData();
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

const DrawData* GetRenderData() {
    return s_renderData;
}

void AddLine(const Vector3f& p_a, const Vector3f& p_b, const Color& p_color) {
    if (DEV_VERIFY(s_renderData)) {
        const Point a = { p_a, p_color };
        const Point b = { p_b, p_color };
        s_renderData->points3D.emplace_back(a);
        s_renderData->points3D.emplace_back(b);
    }
}

void AddLineList(const std::vector<Point>& p_points) {
    if (DEV_VERIFY(s_renderData)) {
        auto& a = s_renderData->points3D;
        const auto& b = p_points;
        a.insert(a.end(), b.begin(), b.end());
    }
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
