#include "engine/renderer/frame_data.h"
#include "engine/scene/scene.h"

namespace my {

void RunMeshRenderSystem(Scene& p_scene, FrameData& p_framedata) {
    p_framedata.FillLightBuffer(p_scene);
    p_framedata.FillVoxelPass(p_scene);
    p_framedata.FillMainPass(p_scene);
}

}  // namespace my
