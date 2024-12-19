#pragma once
#include "engine/core/math/aabb.h"
#include "engine/core/math/geomath.h"
#include "shader_defines.hlsl.h"

namespace YAML {
class Node;
class Emitter;
}  // namespace YAML

namespace my {

class Archive;
class FileAccess;
class TransformComponent;

// @TODO:
// Move Update to elsewhere logic should not be in component
// Split it to different light types?
// Light needs refactor

class LightComponent {
public:
    enum : uint32_t {
        NONE = BIT(0),
        DIRTY = BIT(1),
        CAST_SHADOW = BIT(2),
        SHADOW_REGION = BIT(3),
    };

    bool IsDirty() const { return m_flags & DIRTY; }
    void SetDirty(bool p_dirty = true) { p_dirty ? m_flags |= DIRTY : m_flags &= ~DIRTY; }

    bool CastShadow() const { return m_flags & CAST_SHADOW; }
    void SetCastShadow(bool p_cast = true) { p_cast ? m_flags |= CAST_SHADOW : m_flags &= ~CAST_SHADOW; }

    bool HasShadowRegion() const { return m_flags & SHADOW_REGION; }
    void SetShadowRegion(bool p_region = true) { p_region ? m_flags |= SHADOW_REGION : m_flags &= ~SHADOW_REGION; }

    int GetType() const { return m_type; }
    void SetType(int p_type) { m_type = p_type; }

    float GetMaxDistance() const { return m_maxDistance; }
    int GetShadowMapIndex() const { return m_shadowMapIndex; }

    void Update(const TransformComponent& p_transform);

    void Serialize(Archive& p_archive, uint32_t p_version);

    static void RegisterClass();

    const auto& GetMatrices() const { return m_lightSpaceMatrices; }
    const Vector3f& GetPosition() const { return m_position; }

    struct {
        float constant;
        float linear;
        float quadratic;
    } m_atten;

    AABB m_shadowRegion;

private:
    uint32_t m_flags = DIRTY;
    int m_type = LIGHT_TYPE_INFINITE;

    // Non-serialized
    float m_maxDistance;
    Vector3f m_position;
    int m_shadowMapIndex = -1;
    std::array<Matrix4x4f, 6> m_lightSpaceMatrices;
};

}  // namespace my
