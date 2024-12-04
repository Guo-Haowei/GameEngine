#pragma once

namespace my {
class Scene;
}

namespace my::renderer {

struct RenderData;

void BeginFrame();

void EndFrame();

void RequestScene(Scene& p_scene);

const RenderData* GetRenderData();

}  // namespace my::renderer
