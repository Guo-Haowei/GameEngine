#pragma once

namespace my {

// @TODO: entirely refactor this
class CameraComponent;
class Scene;
class IGraphicsManager;

enum class PathTracerMode {
    NONE,
    INTERACTIVE,
    TILED,
};

void RequestPathTracerUpdate(const CameraComponent& p_camera, Scene& p_scene);
// path tracer
void SetPathTracerMode(PathTracerMode p_mode);
bool IsPathTracerActive();
void BindPathTracerData(IGraphicsManager& p_graphics_manager);
void UnbindPathTracerData(IGraphicsManager& p_graphics_manager);

}  // namespace my
