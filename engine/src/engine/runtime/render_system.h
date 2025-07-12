#pragma once
#include "engine/runtime/module.h"

namespace my {

class CameraComponent;
struct FrameData;
class Scene;

class RenderSystem : public Module {
public:
    RenderSystem()
        : Module("RenderSystem") {}

    void BeginFrame();

    void RenderFrame(Scene& p_scene);

    const FrameData* GetFrameData() const { return m_frameData; }

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    void FillCameraData(const CameraComponent& p_camera, FrameData& p_framedata);

    FrameData* m_frameData{ nullptr };
};

}  // namespace my
