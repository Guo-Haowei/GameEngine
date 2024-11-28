#pragma once
#include "engine/core/math/angle.h"
#include "engine/scene/camera.h"

namespace my {

class CameraController {
public:
    void Move(float p_detla_time, Camera& p_camera, Vector3i& p_move, float p_scroll);
};

}  // namespace my
