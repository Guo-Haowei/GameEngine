#include "renderer.h"

#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/math/geometry.h"
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

static void PrepareDebugDraws() {
    auto& context = s_glob.renderData->debugDrawContext;
    context.drawCount = (uint32_t)context.positions.size();
}

void EndFrame() {
    PrepareDebugDraws();

    s_glob.state = RenderState::SUBMITTING;
}

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene) {
    ASSERT_CAN_RECORD();

    RenderDataConfig config(p_scene);
    config.isOpengl = GraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL;
    PrepareRenderData(p_camera, config, *s_glob.renderData);
}

void RequestBakingIbl() {
    ASSERT_CAN_RECORD();

    if (DEV_VERIFY(s_glob.renderData)) {
        s_glob.renderData->bakeIbl = true;
    }
}

const DrawData* GetRenderData() {
    return s_glob.renderData;
}

void AddDebugCube(const AABB& p_aabb,
                  const Color& p_color,
                  const Matrix4x4f* p_transform) {
    ASSERT_CAN_RECORD();

    const Vector3f& min = p_aabb.GetMin();
    const Vector3f& max = p_aabb.GetMax();

    std::vector<Vector3f> positions;
    std::vector<uint32_t> indices;
    BoxWireFrameHelper(min, max, positions, indices);

    auto& context = s_glob.renderData->debugDrawContext;
    for (const auto& i : indices) {
        const Vector3f& pos = positions[i];
        if (p_transform) {
            context.positions.emplace_back(Vector3f(*p_transform * Vector4f(pos, 1.0f)));
        } else {
            context.positions.emplace_back(Vector3f(pos));
        }
        context.colors.emplace_back(p_color);
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
