#pragma once
#include "engine/core/math/aabb.h"
#include "engine/core/math/color.h"
#include "engine/core/math/vector.h"

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

void AddDebugCube(const AABB& p_aabb,
                  const Color& p_color,
                  const Matrix4x4f* p_transform = nullptr);

void AddImage2D(uint64_t p_handle,
                const NewVector2f& p_size,
                const NewVector2f& p_position = NewVector2f::Zero,
                int p_mode = 0);

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene);

void RequestBakingIbl();

const DrawData* GetRenderData();

PointShadowHandle AllocatePointLightShadowMap();

void FreePointLightShadowMap(PointShadowHandle& p_handle);

}  // namespace my::renderer
