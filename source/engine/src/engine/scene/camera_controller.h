#pragma once
#include "core/math/angle.h"
#include "scene/camera.h"

namespace my {

class CameraController {
public:
    void move(float dt, Camera& camera);
};

}  // namespace my
