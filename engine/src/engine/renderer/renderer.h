#pragma once

namespace my {
class PerspectiveCameraComponent;
class Scene;
}  // namespace my

namespace my::renderer {

struct RenderData;

void BeginFrame();

void EndFrame();

void RequestScene(const PerspectiveCameraComponent& p_camera, Scene& p_scene);

const RenderData* GetRenderData();

}  // namespace my::renderer
