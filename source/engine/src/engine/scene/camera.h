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

    void update();

    void setDimension(int p_width, int p_height);

    bool isDirty() const { return m_flags & DIRTY; }
    void setDirty(bool p_dirty = true) { p_dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }

    Degree getFovy() const { return m_fovy; }
    float getNear() const { return m_near; }
    void setNear(float p_near) { m_near = p_near; }
    float getFar() const { return m_far; }
    void setFar(float p_far) { m_far = p_far; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspect() const { return (float)m_width / m_height; }
    const mat4& getViewMatrix() const { return m_view_matrix; }
    const mat4& getProjectionMatrix() const { return m_projection_matrix; }
    const mat4& getProjectionViewMatrix() const { return m_projection_view_matrix; }
    const vec3& getPosition() const { return m_position; }
    const vec3& getRight() const { return m_right; }
    const vec3 getFront() const { return m_front; }

    void serialize(Archive& p_archive, uint32_t p_version);

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

    mat4 m_view_matrix;
    mat4 m_projection_matrix;
    mat4 m_projection_view_matrix;

    friend class Scene;
    friend class CameraController;
};

}  // namespace my