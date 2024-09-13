#pragma once
#include "core/math/angle.h"
#include "scene/camera.h"

namespace my {

class CameraController {
public:
    void move(float p_detla_time, Camera& p_camera);
};

}  // namespace my
