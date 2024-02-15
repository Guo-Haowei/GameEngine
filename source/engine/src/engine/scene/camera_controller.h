#pragma once
#include "core/math/angle.h"
#include "scene/scene_components.h"

namespace my {

class CameraController {
public:
    void move(float dt, CameraComponent& camera, TransformComponent& transform);

    void setup(const TransformComponent& transform);

private:
    Radians m_pitch;
    Radians m_roll;
    bool m_initialized = false;
};

}  // namespace my
