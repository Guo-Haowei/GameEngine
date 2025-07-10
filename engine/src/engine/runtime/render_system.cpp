#include "render_system.h"

#include "engine/renderer/renderer.h"
#include "engine/runtime/application.h"

namespace my {

auto RenderSystem::InitializeImpl() -> Result<void> {
    return Result<void>();
}

void RenderSystem::FinalizeImpl() {
}

void RenderSystem::RenderFrame(Scene& p_scene) {
    CameraComponent& camera = *m_app->GetActiveCamera();
    renderer::RequestScene(camera, p_scene);
    renderer::EndFrame();
}

void RenderSystem::RunMeshRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    unused(p_scene);
    unused(p_framedata);
}

void RenderSystem::RunTileMapRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    unused(p_scene);
    unused(p_framedata);
}

void RenderSystem::RunSpriteRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    unused(p_scene);
    unused(p_framedata);
}

}  // namespace my
