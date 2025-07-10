#include "renderer.h"

#include "engine/core/debugger/profiler.h"
#include "engine/math/geometry.h"
#include "engine/renderer/frame_data.h"
#include "engine/renderer/graphics_manager.h"
#include "engine/renderer/path_tracer/path_tracer.h"
#include "engine/renderer/sampler.h"
#include "engine/runtime/scene_manager.h"

#define DEFINE_DVAR
#include "graphics_dvars.h"

namespace my {
#include "shader_defines.hlsl.h"
}

namespace my::renderer {

static struct {
    PathTracer pt;
} s_glob;

void RegisterDvars() {
#define REGISTER_DVAR
#include "graphics_dvars.h"
}

void RequestScene(const CameraComponent&, Scene& p_scene) {
    // @TODO: refactor
    s_glob.pt.Update(p_scene);
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
