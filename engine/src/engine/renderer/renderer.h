#pragma once

namespace my {
class CameraComponent;
class Scene;
}

namespace my::renderer {

struct RenderData;

void BeginFrame();

void EndFrame();

void RequestScene(const CameraComponent& p_camera, Scene& p_scene);

const RenderData* GetRenderData();

}  // namespace my::renderer
