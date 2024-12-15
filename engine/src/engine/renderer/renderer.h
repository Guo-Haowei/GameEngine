#pragma once

namespace my {
class PerspectiveCameraComponent;
class Scene;
}  // namespace my

namespace my::renderer {

struct RenderData;

using PointShadowHandle = int;
constexpr PointShadowHandle INVALID_POINT_SHADOW_HANDLE = -1;

void RegisterDvars();

void BeginFrame();

void EndFrame();

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene);

void RequestBakingIbl();

const RenderData* GetRenderData();

PointShadowHandle AllocatePointLightShadowMap();

void FreePointLightShadowMap(PointShadowHandle& p_handle);

}  // namespace my::renderer
