#pragma once
#include "engine/math/aabb.h"
#include "engine/math/color.h"
#include "engine/math/vector.h"

namespace my {

class CameraComponent;
class Scene;
class IGraphicsManager;
struct GpuTexture;
struct RenderSystem;

enum class PathTracerMode {
    NONE,
    INTERACTIVE,
    TILED,
};

}  // namespace my

namespace my::renderer {

void RegisterDvars();

void BeginFrame();

void EndFrame();

void AddDebugCube(const AABB& p_aabb,
                  const Color& p_color,
                  const Matrix4x4f* p_transform = nullptr);

void RequestScene(const CameraComponent& p_camera, Scene& p_scene);

const RenderSystem* GetRenderData();

// path tracer
void SetPathTracerMode(PathTracerMode p_mode);
bool IsPathTracerActive();
void BindPathTracerData(IGraphicsManager& p_graphics_manager);
void UnbindPathTracerData(IGraphicsManager& p_graphics_manager);

}  // namespace my::renderer
