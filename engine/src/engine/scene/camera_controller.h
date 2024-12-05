#pragma once
#include "engine/core/math/geomath.h"

namespace my {

class PerspectiveCameraComponent;

class CameraController {
public:
    void Move(float p_detla_time, PerspectiveCameraComponent& p_camera, Vector3i& p_move, float p_scroll);
};

}  // namespace my
