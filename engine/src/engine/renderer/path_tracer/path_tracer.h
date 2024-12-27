#pragma once
#include "engine/math/box.h"
#include "structured_buffer.hlsl.h"

namespace my {

class Scene;

struct GpuScene {
    std::vector<GpuBvhAccel> bvhs;
    std::vector<GpuTriangleVertex> vertices;
    // @TODO rename
    std::vector<GpuTriangleIndex> triangles;
};

void ConstructScene(const Scene& p_scene, GpuScene& p_gpu_scene);

}  // namespace my
