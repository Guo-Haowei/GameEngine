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

    static constexpr float kDefaultNear = 0.1f;
    static constexpr float kDefaultFar = 100.0f;
    static constexpr Degree kDefaultFovy{ 50.0f };

    void update();

    void set_dimension(int width, int height);

    bool is_dirty() const { return m_flags & DIRTY; }
    void set_dirty(bool dirty = true) { dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }

    Degree get_fovy() const { return m_fovy; }
    float get_near() const { return m_near; }
    void set_near(float z_near) { m_near = z_near; }
    float get_far() const { return m_far; }
    void set_far(float z_far) { m_far = z_far; }
    int get_width() const { return m_width; }
    int get_height() const { return m_height; }
    float get_aspect() const { return (float)m_width / m_height; }
    const mat4& get_view_matrix() const { return m_view_matrix; }
    const mat4& get_projection_matrix() const { return m_projection_matrix; }
    const mat4& get_projection_view_matrix() const { return m_projection_view_matrix; }
    const vec3& get_position() const { return m_position; }
    const vec3& get_right() const { return m_right; }
    const vec3 get_front() const { return m_front; }

    void serialize(Archive& archive, uint32_t version);

private:
    uint32_t m_flags = DIRTY;

    Degree m_fovy{ kDefaultFovy };
    float m_near = kDefaultNear;
    float m_far = kDefaultFar;
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