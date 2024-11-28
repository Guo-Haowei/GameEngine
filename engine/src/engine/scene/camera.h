#pragma once
#include "engine/core/base/noncopyable.h"
#include "engine/core/math/angle.h"
#include "engine/core/math/geomath.h"

namespace my {

class Archive;

// @TODO: make it a component
class Camera : public NonCopyable {
public:
    enum : uint32_t {
        NONE = 0,
        DIRTY = 1,
    };

    static constexpr float DEFAULT_NEAR = 0.1f;
    static constexpr float DEFAULT_FAR = 100.0f;
    static constexpr Degree DEFAULT_FOVY{ 50.0f };

    void Update();

    void SetDimension(int p_width, int p_height);

    bool IsDirty() const { return m_flags & DIRTY; }
    void SetDirty(bool p_dirty = true) { p_dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }

    Degree GetFovy() const { return m_fovy; }
    float GetNear() const { return m_near; }
    void SetNear(float p_near) { m_near = p_near; }
    float GetFar() const { return m_far; }
    void SetFar(float p_far) { m_far = p_far; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    float GetAspect() const { return (float)m_width / m_height; }
    const Matrix4x4f& GetViewMatrix() const { return m_viewMatrix; }
    const Matrix4x4f& GetProjectionMatrix() const { return m_projectionMatrix; }
    const Matrix4x4f& GetProjectionViewMatrix() const { return m_projectionViewMatrix; }
    const Vector3f& GetPosition() const { return m_position; }
    const Vector3f& GetRight() const { return m_right; }
    const Vector3f GetFront() const { return m_front; }

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    uint32_t m_flags = DIRTY;

    Degree m_fovy{ DEFAULT_FOVY };
    float m_near{ DEFAULT_NEAR };
    float m_far{ DEFAULT_FAR };
    int m_width{ 0 };
    int m_height{ 0 };
    Degree m_pitch;  // x-axis
    Degree m_yaw;    // y-axis
    Vector3f m_position{ 0 };

    // Non-serlialized
    Vector3f m_front;
    Vector3f m_right;

    Matrix4x4f m_viewMatrix;
    Matrix4x4f m_projectionMatrix;
    Matrix4x4f m_projectionViewMatrix;

    friend class Scene;
    friend class CameraController;
};

}  // namespace my