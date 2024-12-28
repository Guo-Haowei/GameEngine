#pragma once
#include "engine/math/aabb.h"
#include "engine/math/vector.h"

namespace my {
#include "structured_buffer.hlsl.h"
}  // namespace my

namespace my {

struct BvhAccel {
    using Ref = std::shared_ptr<BvhAccel>;

    BvhAccel(int p_index,
             const BvhAccel* p_parent) : index(p_index),
                                         parent(p_parent) {}

    void DiscoverIdx();

    AABB aabb;

    const int index;
    const BvhAccel* parent;

    int triangleIndex{ -1 };
    int hitIndex{ -1 };
    int missIndex{ -1 };
    int depth{ 0 };
    Ref left{ nullptr };
    Ref right{ nullptr };
    bool isLeaf{ false };

    static Ref Construct(const std::vector<uint32_t>& p_indices,
                         const std::vector<Vector3f>& p_vertices);

    void FillGpuBvhAccel(int p_mesh_index, std::vector<GpuBvhAccel>& p_out);
};

}  // namespace my
