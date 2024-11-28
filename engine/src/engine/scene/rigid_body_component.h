#pragma once
#include "engine/core/math/geomath.h"

namespace my {

class Archive;

struct RigidBodyComponent {
    enum CollisionShape {
        SHAPE_UNKNOWN,
        SHAPE_SPHERE,
        SHAPE_CUBE,
        SHAPE_MAX,
    };

    union Parameter {
        struct {
            Vector3f half_size;
        } box;
        struct {
            float radius;
        } sphere;
    };

    float mass = 1.0f;
    CollisionShape shape;
    Parameter param;

    void Serialize(Archive& p_archive, uint32_t p_version);
};

}  // namespace my