#pragma once
#include "engine/runtime/module.h"

namespace my {

class CameraComponent;
struct FrameData;
class Scene;

class RenderSystem : public Module {
public:
    RenderSystem() : Module("RenderSystem") {}

    void RenderFrame(Scene& p_scene);

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    void RunTileMapRenderSystem(Scene& p_scene, FrameData& p_framedata);
    void RunSpriteRenderSystem(Scene& p_scene, FrameData& p_framedata);

    void FillCameraData(const CameraComponent& p_camera, FrameData& p_framedata);
};

}  // namespace my
