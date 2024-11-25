#pragma once
#include "core/math/box.h"
#include "structured_buffer.hlsl.h"

namespace my {

using GpuBvhList = std::vector<gpu_bvh_t>;
using GeometryList = std::vector<gpu_geometry_t>;

class Scene;

class Bvh {
public:
    Bvh() = delete;
    explicit Bvh(GeometryList& geoms, Bvh* parent = nullptr);
    ~Bvh();

    void CreateGpuBvh(GpuBvhList& outBvh, GeometryList& outTriangles);
    inline const Box3& GetBox() const { return m_box; }

private:
    void SplitByAxis(GeometryList& geoms);
    void DiscoverIdx();

    Box3 m_box;
    gpu_geometry_t m_geom;
    Bvh* m_left;
    Bvh* m_right;
    bool m_leaf;

    const int m_idx;
    Bvh* const m_parent;

    int m_hitIdx;
    int m_missIdx;
};

struct SceneGeometry {
    enum class Kind {
        Invalid,
        Sphere,
        Quad,
        Cube,
        Mesh,
    };

    SceneGeometry()
        : kind(Kind::Invalid), materidId(-1), translate(Vector3f(0)), euler(Vector3f(0)), scale(Vector3f(1)) {}

    std::string name;
    std::string path;
    Kind kind;
    int materidId;
    Vector3f translate;
    Vector3f euler;
    Vector3f scale;
};

struct GpuScene {
    std::vector<gpu_material_t> materials;
    std::vector<gpu_geometry_t> geometries;
    std::vector<gpu_bvh_t> bvhs;

    int height;
    Box3 bbox;
};

void ConstructScene(const Scene& inScene, GpuScene& outScene);

}  // namespace my
