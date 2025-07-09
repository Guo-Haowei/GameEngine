#pragma once
#include "engine/math/geomath.h"
#include "engine/scene/scriptable_entity.h"

namespace my {

class CameraComponent;

struct CameraInputState {
    Vector3f move{ 0, 0, 0 };
    float zoomDelta{ 0 };
    Vector2f rotation{ 0, 0 };
};

class CameraControllerFPS {
public:
    void Update(CameraComponent& p_camera, const CameraInputState& p_state);

    float m_moveSpeed{ 10.0f };
    float m_rotateSpeed{ 10.0f };
    float m_scrollSpeed{ 2.0f };
};

class CameraController2DEditor {
public:
    // @TODO: implement
};

// class EditorCameraController : public ScriptableEntity {
// public:
//     void Move(const CameraInputState& p_context);
//
//     void SetMoveSpeed(float p_speed) { m_moveSpeed = p_speed; }
//
//     void SetRotateSpeed(float p_speed) { m_rotateSpeed = p_speed; }
//
//     void SetScrollSpeed(float p_speed) { m_scrollSpeed = p_speed; }
//
// private:
//     void OnCreate() override;
//
//     void OnUpdate(float p_timestep) override;
//
// };

}  // namespace my
