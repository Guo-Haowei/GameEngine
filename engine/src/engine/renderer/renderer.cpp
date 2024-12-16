#include "renderer.h"

#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/renderer/draw_data.h"

#define DEFINE_DVAR
#include "graphics_dvars.h"

namespace my::renderer {

#define ASSERT_CAN_RECORD() DEV_ASSERT(s_glob.state == RenderState::RECORDING)

enum class RenderState {
    UNINITIALIZED = 0,
    RECORDING,
    SUBMITTING,
};

static struct {
    DrawData* renderData;
    RenderState state;
} s_glob;

// @TODO: fix this
std::list<PointShadowHandle> s_freePointLightShadows = { 0, 1, 2, 3, 4, 5, 6, 7 };

void RegisterDvars() {
#define REGISTER_DVAR
#include "graphics_dvars.h"
}

void BeginFrame() {
    // @TODO: should be a better way
    if (s_glob.renderData) {
        delete s_glob.renderData;
        s_glob.renderData = nullptr;
    }
    s_glob.renderData = new DrawData();
    s_glob.renderData->bakeIbl = false;

    s_glob.state = RenderState::RECORDING;
}

static void BakeLineSegments() {
}

void EndFrame() {
    BakeLineSegments();

    s_glob.state = RenderState::SUBMITTING;
}

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene) {
    ASSERT_CAN_RECORD();

    RenderDataConfig config(p_scene);
    config.isOpengl = GraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;
    PrepareRenderData(p_camera, config, *s_glob.renderData);
}

void RequestBakingIbl() {
    if (DEV_VERIFY(s_glob.renderData)) {
        s_glob.renderData->bakeIbl = true;
    }
}

const DrawData* GetRenderData() {
    return s_glob.renderData;
}

void AddLine(const Vector3f& p_a,
             const Vector3f& p_b,
             const Color& p_color,
             float p_thickness) {
    ASSERT_CAN_RECORD();

    if (DEV_VERIFY(s_glob.renderData)) {
        s_glob.renderData->lineContext.lines.push_back({ p_a, p_b, p_thickness, p_color });
    }
}

void AddLineList(const std::vector<Vector3f>& p_points,
                 const Color& p_color,
                 const Matrix4x4f* p_transform,
                 float p_thickness) {
    ASSERT_CAN_RECORD();

    if (DEV_VERIFY(s_glob.renderData)) {
        for (size_t i = 0; i < p_points.size(); i += 2) {
            Vector4f a(p_points[i], 1.0f);
            Vector4f b(p_points[i + 1], 1.0f);
            if (p_transform) {
                a = *p_transform * a;
                b = *p_transform * b;
            }
            s_glob.renderData->lineContext.lines.push_back({ a, b, p_thickness, p_color });
        }
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
