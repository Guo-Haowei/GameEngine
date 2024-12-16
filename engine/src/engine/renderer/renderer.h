#pragma once
#include "engine/core/math/color.h"

namespace my {
class PerspectiveCameraComponent;
class Scene;
}  // namespace my

namespace my::renderer {

struct DrawData;

using PointShadowHandle = int;
constexpr PointShadowHandle INVALID_POINT_SHADOW_HANDLE = -1;

void RegisterDvars();

void BeginFrame();

void EndFrame();

void AddLine(const Vector3f& p_a,
             const Vector3f& p_b,
             const Color& p_color,
             float p_thickness = 1.0f);

void AddLineList(const std::vector<Vector3f>& p_points,
                 const Color& p_color,
                 const Matrix4x4f* p_transform = nullptr,
                 float p_thickness = 1.0f);

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene);

void RequestBakingIbl();

const DrawData* GetRenderData();

PointShadowHandle AllocatePointLightShadowMap();

void FreePointLightShadowMap(PointShadowHandle& p_handle);

}  // namespace my::renderer
