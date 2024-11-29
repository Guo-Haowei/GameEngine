#pragma once

namespace my {
class Application;
}

namespace my::renderer {

struct RenderData;

void BeginFrame(Application* p_app);

void EndFrame();

const RenderData* GetRenderData();

}  // namespace my::renderer
