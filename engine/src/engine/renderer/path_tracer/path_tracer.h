#pragma once
#include "engine/math/box.h"
#include "engine/renderer/gpu_resource.h"
#include "engine/renderer/path_tracer/bvh_accel.h"
#include "engine/renderer/renderer.h"
#include "engine/systems/ecs/entity.h"

namespace my {

class Scene;

struct GpuScene {
    // @TODO: material
    std::vector<GpuPtBvh> bvhs;
    std::vector<GpuPtVertex> vertices;
    std::vector<GpuPtIndex> indices;
};

// @TODO: make it a layer?
class PathTracer {
public:
    void SetMode(PathTracerMode p_mode) { m_mode = p_mode; }

    void Update(const Scene& p_scene);

    bool IsActive() const;

    void BindData(IGraphicsManager& p_gm);

    void UnbindData(IGraphicsManager& p_gm);

private:
    bool CreateAccelStructure(const Scene& p_scene);
    void UpdateAccelStructure(const Scene& p_scene);

    std::shared_ptr<GpuStructuredBuffer> m_ptBvhBuffer;
    std::shared_ptr<GpuStructuredBuffer> m_ptVertexBuffer;
    std::shared_ptr<GpuStructuredBuffer> m_ptIndexBuffer;
    std::shared_ptr<GpuStructuredBuffer> m_ptMeshBuffer;
    std::shared_ptr<GpuStructuredBuffer> m_ptMaterialBuffer;

    struct MeshData {
        int rootBvhId;
        ecs::Entity materialId;
    };

    // @TODO: rename
    std::map<ecs::Entity, MeshData> m_meshs;

    PathTracerMode m_mode{ PathTracerMode::NONE };
};

}  // namespace my
