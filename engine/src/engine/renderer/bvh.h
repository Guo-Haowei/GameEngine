#pragma once
#include "engine/math/aabb.h"
#include "engine/math/vector.h"

namespace my {

struct BVH {
    using Ref = std::shared_ptr<BVH>;

    math::AABB aabb;
    Vector3i indices{ -1 };  // indices of triangle points A, B, C

    // @TODO: refactor to use this
    int triangleCount{ 0 };
    int triangleOffset{ -1 };

    int depth{ 0 };
    Ref left{ nullptr };
    Ref right{ nullptr };
    bool isLeaf{ false };

    static Ref Construct(const std::vector<uint32_t>& p_indices,
                         const std::vector<Vector3f>& p_vertices,
                         int p_max_depth = 32);
};

}  // namespace my
