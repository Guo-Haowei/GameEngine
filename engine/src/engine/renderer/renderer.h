#pragma once
#include "engine/core/math/color.h"

namespace my {
class PerspectiveCameraComponent;
class Scene;

struct Point {
    Vector3f position;
    Color color;
};

}  // namespace my

namespace my::renderer {

struct DrawData;

using PointShadowHandle = int;
constexpr PointShadowHandle INVALID_POINT_SHADOW_HANDLE = -1;

void RegisterDvars();

void BeginFrame();

void EndFrame();

void AddLine(const Vector3f& p_a, const Vector3f& p_b, const Color& p_color);

void AddLineList(const std::vector<Point>& p_points);

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene);

void RequestBakingIbl();

const DrawData* GetRenderData();

PointShadowHandle AllocatePointLightShadowMap();

void FreePointLightShadowMap(PointShadowHandle& p_handle);

}  // namespace my::renderer
