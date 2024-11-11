#pragma once
#include "core/math/angle.h"
#include "scene/camera.h"

namespace my {

class CameraController {
public:
    void Move(float p_detla_time, Camera& p_camera);
};

}  // namespace my
