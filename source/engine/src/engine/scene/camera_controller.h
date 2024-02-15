#pragma once
#include "core/math/degree.h"
#include "scene/scene_components.h"

namespace vct {

class CameraController {
public:
    static constexpr float MAX_SCROLL_SPEED = 100.0f;

    void move(float dt, CameraComponent& camera, TransformComponent& transform);

private:
    float m_scroll_speed = 0.0f;
};

}  // namespace vct
