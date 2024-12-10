#pragma once
#include "engine/core/base/rid.h"
#include "engine/core/math/aabb.h"
#include "engine/core/systems/entity.h"

namespace my {

class Archive;
class Scene;
struct MeshBuffers;

struct MeshComponent {
    enum : uint32_t {
        NONE = BIT(0),
        RENDERABLE = BIT(1),
        DOUBLE_SIDED = BIT(2),
        DYNAMIC = BIT(3),
    };

    struct VertexAttribute {
        enum NAME {
            POSITION = 0,
            NORMAL,
            TEXCOORD_0,
            TEXCOORD_1,
            TANGENT,
            JOINTS_0,
            WEIGHTS_0,
            COLOR_0,
            COUNT,
        } name;

        uint32_t offset_in_byte = 0;
        uint32_t size_in_byte = 0;
        uint32_t stride = 0;

        bool IsValid() const { return size_in_byte != 0; }
    };

    uint32_t flags = RENDERABLE;
    std::vector<uint32_t> indices;
    std::vector<Vector3f> positions;
    std::vector<Vector3f> normals;
    std::vector<Vector3f> tangents;
    std::vector<Vector2f> texcoords_0;
    std::vector<Vector2f> texcoords_1;
    std::vector<Vector4i> joints_0;
    std::vector<Vector4f> weights_0;
    std::vector<Vector3f> color_0;

    struct MeshSubset {
        ecs::Entity material_id;
        uint32_t index_offset = 0;
        uint32_t index_count = 0;
        AABB local_bound;
    };
    std::vector<MeshSubset> subsets;

    ecs::Entity armatureId;

    // Non-serialized
    mutable const MeshBuffers* gpuResource = nullptr;
    AABB localBound;

    mutable std::vector<Vector3f> updatePositions;
    mutable std::vector<Vector3f> updateNormals;

    VertexAttribute attributes[VertexAttribute::COUNT];
    size_t vertexBufferSize = 0;  // combine vertex buffer

    void CreateRenderData();
    std::vector<char> GenerateCombinedBuffer() const;

    void Serialize(Archive& p_archive, uint32_t p_version);
};

}  // namespace my