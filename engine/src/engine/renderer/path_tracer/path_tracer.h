#pragma once
#include "engine/math/box.h"
#include "engine/renderer/bvh_accel.h"

namespace my {

class Scene;

struct GpuScene {
    // @TODO: material
    std::vector<GpuPtBvh> bvhs;
    std::vector<GpuPtVertex> vertices;
    std::vector<GpuPtMesh> meshes;
    std::vector<Vector3i> indices;
};

void ConstructScene(const Scene& p_scene, GpuScene& p_gpu_scene);

}  // namespace my
