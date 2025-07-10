#pragma once
#include "engine/math/aabb.h"
#include "engine/math/color.h"
#include "engine/math/vector.h"

namespace my {

class CameraComponent;
class Scene;
class IGraphicsManager;
struct GpuTexture;
struct FrameData;

enum class PathTracerMode {
    NONE,
    INTERACTIVE,
    TILED,
};

}  // namespace my

// @TODO: refactor
namespace my::renderer {

void RegisterDvars();

void RequestScene(const CameraComponent& p_camera, Scene& p_scene);

// path tracer
void SetPathTracerMode(PathTracerMode p_mode);
bool IsPathTracerActive();
void BindPathTracerData(IGraphicsManager& p_graphics_manager);
void UnbindPathTracerData(IGraphicsManager& p_graphics_manager);

}  // namespace my::renderer
