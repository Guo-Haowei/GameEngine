#pragma once
#include "engine/math/aabb.h"
#include "engine/math/color.h"
#include "engine/math/vector.h"

namespace my {

class PerspectiveCameraComponent;
class Scene;
class IGraphicsManager;
struct GpuTexture;

enum class PathTracerMode {
    NONE,
    INTERACTIVE,
    TILED,
};

}  // namespace my

namespace my::renderer {

struct RenderSystem;

using PointShadowHandle = int;
constexpr PointShadowHandle INVALID_POINT_SHADOW_HANDLE = -1;

void RegisterDvars();

void BeginFrame();

void EndFrame();

void AddDebugCube(const AABB& p_aabb,
                  const Color& p_color,
                  const Matrix4x4f* p_transform = nullptr);

void AddImage2D(GpuTexture* p_texture,
                const Vector2f& p_size,
                const Vector2f& p_position = Vector2f::Zero,
                int p_mode = 0);

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene);

void RequestBakingIbl();

const RenderSystem* GetRenderData();

PointShadowHandle AllocatePointLightShadowMap();

void FreePointLightShadowMap(PointShadowHandle& p_handle);

// path tracer
void SetPathTracerMode(PathTracerMode p_mode);
bool IsPathTracerActive();
void BindPathTracerData(IGraphicsManager& p_graphics_manager);
void UnbindPathTracerData(IGraphicsManager& p_graphics_manager);

// resources
// @TODO: release resources
using KernelData = std::array<Vector4f, 64>;

Result<void> CreateResources(IGraphicsManager& p_graphics_manager);

const KernelData& GetKernelData();
std::shared_ptr<GpuTexture> GetSsaoNoiseTexture();

}  // namespace my::renderer
