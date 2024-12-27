#include "bvh_accel.h"

#include <algorithm>

namespace my {

using math::AABB;

using VertexList = std::vector<Vector3f>;
using TriangleList = std::vector<Vector3i>;

class BvhSorter;

class BvhBuilder {
public:
    BvhBuilder(const VertexList& p_vertices,
               const TriangleList& p_triangles);

    BvhAccel::Ref ConstructHelper(const BvhAccel* p_parent, const std::vector<uint32_t>& p_indices) const;

    uint32_t GetBVHCount() const {
        return m_bvhCounter;
    }

private:
    BvhBuilder(const BvhBuilder&) = delete;

    void SplitByAxis(BvhAccel* p_parent, const std::vector<uint32_t>& p_indices) const;
    AABB AABBFromTriangles(const std::vector<uint32_t>& p_indices) const;

    mutable uint32_t m_bvhCounter = 0;
    const VertexList& m_vertices;
    const TriangleList& m_triangles;
    std::vector<AABB> m_aabbs;
    std::vector<Vector3f> m_centroids;

    friend class BvhSorter;
};

class BvhSorter {
public:
    BvhSorter(int p_axis,
              const BvhBuilder& p_builder) : m_axis(p_axis),
                                             m_builder(p_builder) {}

    bool operator()(uint32_t p_lhs, uint32_t p_rhs) const {
        auto aabb_1 = m_builder.m_aabbs.at(p_lhs);
        auto aabb_2 = m_builder.m_aabbs.at(p_rhs);
        Vector3f center_1 = aabb_1.Center();
        Vector3f center_2 = aabb_2.Center();
        return center_1[m_axis] < center_2[m_axis];
    }

private:
    const int m_axis;
    const BvhBuilder& m_builder;
};

BvhBuilder::BvhBuilder(const VertexList& p_vertices,
                       const TriangleList& p_triangles) : m_vertices(p_vertices),
                                                          m_triangles(p_triangles) {
    m_aabbs.resize(m_triangles.size());
    m_centroids.resize(m_triangles.size());
    for (size_t i = 0; i < m_triangles.size(); ++i) {
        const auto& triangle = m_triangles[i];
        const Vector3f& a = m_vertices.at(triangle.x);
        const Vector3f& b = m_vertices.at(triangle.y);
        const Vector3f& c = m_vertices.at(triangle.z);
        m_centroids[i] = (1.0f / 3.0f) * (a + b + c);
        m_aabbs[i].MakeInvalid();
        m_aabbs[i].ExpandPoint(a);
        m_aabbs[i].ExpandPoint(b);
        m_aabbs[i].ExpandPoint(c);
        m_aabbs[i].MakeValid();
    }
}

static int DominantAxis(const AABB& p_aabb) {
    const Vector3f span = p_aabb.Size();
    int axis = 0;
    if (span[axis] < span.y) {
        axis = 1;
    }
    if (span[axis] < span.z) {
        axis = 2;
    }

    return axis;
}

AABB BvhBuilder::AABBFromTriangles(const std::vector<uint32_t>& p_indices) const {
    AABB aabb;
    for (uint32_t index : p_indices) {
        const auto& points = m_triangles[index];
        aabb.ExpandPoint(m_vertices[points.x]);
        aabb.ExpandPoint(m_vertices[points.y]);
        aabb.ExpandPoint(m_vertices[points.z]);
    }
    aabb.MakeValid();
    return aabb;
}

void BvhBuilder::SplitByAxis(BvhAccel* p_parent,
                             const std::vector<uint32_t>& p_indices) const {
    auto indices = p_indices;
    const int axis = DominantAxis(p_parent->aabb);
    BvhSorter sorter(axis, *this);
    std::sort(indices.begin(), indices.end(), sorter);
    const size_t mid = indices.size() / 2;
    std::vector<uint32_t> left(indices.begin(), indices.begin() + mid);
    std::vector<uint32_t> right(indices.begin() + mid, indices.end());

    p_parent->left = ConstructHelper(p_parent, left);
    p_parent->right = ConstructHelper(p_parent, right);
}

BvhAccel::Ref BvhBuilder::ConstructHelper(const BvhAccel* p_parent, const std::vector<uint32_t>& p_indices) const {
    const int depth = p_parent ? p_parent->depth + 1 : 0;
    if (depth > 32) {
        CRASH_NOW_MSG("TOO MANY LEVELS OF BVH");
        return nullptr;
    }

    const int triangle_count = (int)p_indices.size();
    const AABB parent_aabb = AABBFromTriangles(p_indices);
    auto bvh = std::make_shared<BvhAccel>(m_bvhCounter++, p_parent);
    bvh->aabb = parent_aabb;
    bvh->depth = depth;

    if (triangle_count == 1) {
        bvh->isLeaf = true;
        bvh->triangleIndex = p_indices[0];
        return bvh;
    }

    // @TODO: refactor
    const float parent_surface = parent_aabb.SurfaceArea();

    // @TODO rework
    if (triangle_count <= 4 || parent_surface == 0.0f) {
        SplitByAxis(bvh.get(), p_indices);
        return bvh;
    }

    constexpr int BUCKED_MAX = 12;
    struct BucketInfo {
        int count = 0;
        AABB box;
    };
    std::array<BucketInfo, BUCKED_MAX> buckets;

    AABB centroidBox;
    for (const auto index : p_indices) {
        const Vector3f& point = m_centroids.at(index);
        centroidBox.ExpandPoint(point);
    }
    centroidBox.MakeValid();

    const int axis = DominantAxis(centroidBox);
    const float tmin = centroidBox.GetMin()[axis];
    const float tmax = centroidBox.GetMax()[axis];

    for (int index : p_indices) {
        float tmp = ((m_centroids.at(index)[axis] - tmin) * BUCKED_MAX) / (tmax - tmin);
        int slot = static_cast<int>(tmp);
        slot = math::clamp(slot, 0, BUCKED_MAX - 1);
        BucketInfo& bucket = buckets[slot];
        ++bucket.count;
        bucket.box.UnionBox(m_aabbs.at(index));
    }

    float costs[BUCKED_MAX - 1];
    for (int i = 0; i < BUCKED_MAX - 1; ++i) {
        AABB b0, b1;
        int count0 = 0, count1 = 0;
        for (int j = 0; j <= i; ++j) {
            b0.UnionBox(buckets[j].box);
            count0 += buckets[j].count;
        }
        for (int j = i + 1; j < BUCKED_MAX; ++j) {
            b1.UnionBox(buckets[j].box);
            count1 += buckets[j].count;
        }

        constexpr float travCost = 0.125f;
        costs[i] = travCost + (count0 * b0.SurfaceArea() + count1 * b1.SurfaceArea()) / parent_surface;
    }

    int splitIndex = 0;
    float minCost = costs[splitIndex];
    for (int i = 0; i < BUCKED_MAX - 1; ++i) {
        // printf("cost of split after bucket %d is %f\n", i, costs[i]);
        if (costs[i] < minCost) {
            splitIndex = i;
            minCost = costs[splitIndex];
        }
    }

    // printf("split index is %d\n", splitIndex);

    std::vector<uint32_t> leftPartition;
    std::vector<uint32_t> rightPartition;

    for (auto index : p_indices) {
        const auto& t = m_centroids.at(index);
        float tmp = (t[axis] - tmin) / (tmax - tmin);
        tmp *= BUCKED_MAX;
        int slot = static_cast<int>(tmp);
        slot = glm::clamp(slot, 0, BUCKED_MAX - 1);
        if (slot <= splitIndex) {
            leftPartition.push_back(index);
        } else {
            rightPartition.push_back(index);
        }
    }

    bvh->left = ConstructHelper(bvh.get(), leftPartition);
    bvh->right = ConstructHelper(bvh.get(), rightPartition);
    return bvh;
}

void BvhAccel::DiscoverIdx() {
    missIndex = -1;

    // hit link (find right link)
    for (const BvhAccel* cursor = parent; cursor; cursor = cursor->parent) {
        if (cursor->right && cursor->right->index > index) {
            missIndex = cursor->right->index;
            break;
        }
    }

    // if it's a hit, then check the left child node
    // if there's not left child, treat it as a miss, so next bounding box to check is the miss node
    hitIndex = left ? left->index : missIndex;
}

void BvhAccel::FillGpuBvhAccel(int p_mesh_index, std::vector<GpuBvhAccel>& p_out) {
    DiscoverIdx();
    DEV_ASSERT(aabb.IsValid());

    GpuBvhAccel gpu_bvh;
    gpu_bvh.min = aabb.GetMin();
    gpu_bvh.max = aabb.GetMax();
    gpu_bvh.hitIdx = hitIndex;
    gpu_bvh.missIdx = missIndex;
    gpu_bvh.leaf = !!isLeaf;
    gpu_bvh.meshIndex = p_mesh_index;
    gpu_bvh.triangleIndex = -1;
    if (isLeaf) {
        DEV_ASSERT(triangleIndex != -1);
        gpu_bvh.triangleIndex = triangleIndex;
    }

    p_out.push_back(gpu_bvh);
    if (left) {
        left->FillGpuBvhAccel(p_mesh_index, p_out);
    }
    if (right) {
        right->FillGpuBvhAccel(p_mesh_index, p_out);
    }
}

BvhAccel::Ref BvhAccel::Construct(const std::vector<uint32_t>& p_indices,
                                  const VertexList& p_vertices) {

    const int index_count = (int)p_indices.size();
    DEV_ASSERT(index_count % 3 == 0);
    const int triangle_count = index_count / 3;

    TriangleList triangles(triangle_count);
    std::vector<uint32_t> triangle_indices(triangle_count);

    // @TODO: centroid?
    for (int i = 0; i < index_count; i += 3) {
        const int index = i / 3;

        const int a = p_indices[i];
        const int b = p_indices[i + 1];
        const int c = p_indices[i + 2];
        triangles[index] = Vector3i(a, b, c);
        triangle_indices[index] = index;
    }

    BvhBuilder builder(p_vertices, triangles);
    return builder.ConstructHelper(nullptr, triangle_indices);
}

}  // namespace my
