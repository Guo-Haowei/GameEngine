#pragma once
#include "engine/math/geomath.h"
#include "engine/scene/scriptable_entity.h"

namespace my {

class CameraComponent;

class EditorCameraController : public ScriptableEntity {
public:
    struct Context {
        float timestep;
        float scroll;
        CameraComponent* camera;
        Vector3i move;
        Vector2f rotation;
    };

    void Move(const Context& p_context);

    void SetMoveSpeed(float p_speed) { m_moveSpeed = p_speed; }

    void SetRotateSpeed(float p_speed) { m_rotateSpeed = p_speed; }

    void SetScrollSpeed(float p_speed) { m_scrollSpeed = p_speed; }

private:
    void OnCreate() override;

    void OnUpdate(float p_timestep) override;

    float m_moveSpeed{ 10.0f };
    float m_rotateSpeed{ 10.0f };
    float m_scrollSpeed{ 2.0f };
};

}  // namespace my
