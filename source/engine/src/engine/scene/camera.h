#pragma once
#include "core/base/noncopyable.h"
#include "core/math/angle.h"
#include "core/math/geomath.h"

namespace my {

class Archive;

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
    const mat4& GetViewMatrix() const { return m_viewMatrix; }
    const mat4& GetProjectionMatrix() const { return m_projectionMatrix; }
    const mat4& GetProjectionViewMatrix() const { return m_projectionViewMatrix; }
    const vec3& GetPosition() const { return m_position; }
    const vec3& GetRight() const { return m_right; }
    const vec3 GetFront() const { return m_front; }

    void Serialize(Archive& p_archive, uint32_t p_version);

private:
    uint32_t m_flags = DIRTY;

    Degree m_fovy{ DEFAULT_FOVY };
    float m_near = DEFAULT_NEAR;
    float m_far = DEFAULT_FAR;
    int m_width = 0;
    int m_height = 0;
    Degree m_pitch;  // x-axis
    Degree m_yaw;    // y-axis
    vec3 m_position{ 0 };

    // Non-serlialized
    vec3 m_front;
    vec3 m_right;

    mat4 m_viewMatrix;
    mat4 m_projectionMatrix;
    mat4 m_projectionViewMatrix;

    friend class Scene;
    friend class CameraController;
};

}  // namespace my