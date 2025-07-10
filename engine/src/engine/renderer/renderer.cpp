#include "renderer.h"

#include "engine/core/debugger/profiler.h"
#include "engine/math/geometry.h"
#include "engine/renderer/graphics_manager.h"
#include "engine/renderer/path_tracer/path_tracer.h"
#include "engine/renderer/render_system.h"
#include "engine/renderer/sampler.h"
#include "engine/runtime/scene_manager.h"

#define DEFINE_DVAR
#include "graphics_dvars.h"

namespace my {
#include "shader_defines.hlsl.h"
}

namespace my::renderer {

#define ASSERT_CAN_RECORD() DEV_ASSERT(s_glob.state == RenderState::RECORDING)

enum class RenderState {
    UNINITIALIZED = 0,
    RECORDING,
    SUBMITTING,
};

static struct {
    FrameData* renderData;
    RenderState state;
    PathTracer pt;
    // @TODO: refactor
} s_glob;

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

    RenderOptions options = {
        .isOpengl = IGraphicsManager::GetSingleton().GetBackend() == Backend::OPENGL,
        .ssaoEnabled = DVAR_GET_BOOL(gfx_ssao_enabled),
        .vxgiEnabled = false,
        .bloomEnabled = DVAR_GET_BOOL(gfx_enable_bloom),
        .iblEnabled = DVAR_GET_BOOL(gfx_enable_ibl),
        .debugVoxelId = DVAR_GET_INT(gfx_debug_vxgi_voxel),
        .debugBvhDepth = DVAR_GET_INT(gfx_bvh_debug),
        .voxelTextureSize = DVAR_GET_INT(gfx_voxel_size),
        .ssaoKernelRadius = DVAR_GET_FLOAT(gfx_ssao_radius),
    };

    s_glob.renderData = new FrameData(options);
    static bool firstFrame = true;
    s_glob.renderData->bakeIbl = firstFrame;
    firstFrame = false;
    s_glob.state = RenderState::RECORDING;
}

static void PrepareDebugDraws() {
    auto& context = s_glob.renderData->drawDebugContext;
    context.drawCount = (uint32_t)context.positions.size();
}

void EndFrame() {
    PrepareDebugDraws();

    s_glob.state = RenderState::SUBMITTING;
}

void RequestScene(const CameraComponent& p_camera, Scene& p_scene) {
    HBN_PROFILE_EVENT();

    ASSERT_CAN_RECORD();

    PrepareRenderData(p_camera, p_scene, *s_glob.renderData);

    // @TODO: refactor
    s_glob.pt.Update(p_scene);
}

const FrameData* GetRenderData() {
    return s_glob.renderData;
}

void AddDebugCube(const AABB& p_aabb,
                  const Color& p_color,
                  const Matrix4x4f* p_transform) {
    ASSERT_CAN_RECORD();

    const auto& min = p_aabb.GetMin();
    const auto& max = p_aabb.GetMax();

    std::vector<Vector3f> positions;
    std::vector<uint32_t> indices;
    BoxWireFrameHelper(min, max, positions, indices);

    auto& context = s_glob.renderData->drawDebugContext;
    for (const auto& i : indices) {
        const Vector3f& pos = positions[i];
        if (p_transform) {
            const auto tmp = *p_transform * Vector4f(pos, 1.0f);
            context.positions.emplace_back(Vector3f(tmp.xyz));
        } else {
            context.positions.emplace_back(Vector3f(pos));
        }
        context.colors.emplace_back(p_color);
    }
}

// path tracer
void SetPathTracerMode(PathTracerMode p_mode) {
    s_glob.pt.SetMode(p_mode);
}

bool IsPathTracerActive() {
    return s_glob.pt.IsActive();
}

void BindPathTracerData(IGraphicsManager& p_graphics_manager) {
    s_glob.pt.BindData(p_graphics_manager);
}

void UnbindPathTracerData(IGraphicsManager& p_graphics_manager) {
    s_glob.pt.UnbindData(p_graphics_manager);
}

}  // namespace my::renderer
