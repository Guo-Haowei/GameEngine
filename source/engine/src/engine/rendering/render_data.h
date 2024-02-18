#pragma once
#include "gl_utils.h"
#include "scene/scene.h"

namespace my {

struct RenderData {
    using FilterObjectFunc1 = std::function<bool(const ObjectComponent& object)>;
    using FilterObjectFunc2 = std::function<bool(const AABB& object_aabb)>;

    struct SubMesh {
        uint32_t index_count;
        uint32_t index_offset;
        const MaterialComponent* material;
        uint32_t flags;
    };

    struct Mesh {
        ecs::Entity armature_id;
        mat4 world_matrix;
        const MeshData* mesh_data;
        std::vector<SubMesh> subsets;
    };

    struct Pass {
        mat4 projection_view_matrix;
        std::vector<Mesh> draws;

        void clear() { draws.clear(); }
    };

    const Scene* scene = nullptr;

    // @TODO: save pass item somewhere and use index instead of keeping many copies
    std::array<Pass, NUM_CASCADE_MAX> shadow_passes;
    Pass voxel_pass;
    Pass main_pass;

    void update(const Scene* p_scene);

private:
    void clear();

    void fill(const Scene* p_scene, const mat4& projection_view_matrix, Pass& pass, FilterObjectFunc1 func1, FilterObjectFunc2 func2);
};

}  // namespace my