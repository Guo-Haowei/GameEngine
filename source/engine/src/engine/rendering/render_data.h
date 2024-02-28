#pragma once
#include "core/base/fixed_stack.h"
#include "gl_utils.h"
#include "rendering/r_cbuffers.h"
#include "scene/scene.h"

namespace my {

struct MeshBuffers;

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
        // @TODO: world_matrix index

        ecs::Entity tmp_armature_id;
        const MeshBuffers* mesh_data;
        std::vector<SubMesh> subsets;

        uint32_t batch_buffer_id;
    };

    struct Pass {
        // @TODO: index instead of actuall data

        mat4 projection_matrix;
        mat4 view_matrix;
        mat4 projection_view_matrix;

        void fill_perpass(PerPassConstantBuffer& buffer) {
            buffer.g_projection = projection_matrix;
            buffer.g_view = view_matrix;
            buffer.g_projection_view = projection_view_matrix;
        }

        std::vector<Mesh> draws;
        int light_index;

        void clear() { draws.clear(); }
    };

    struct LightBillboard {
        mat4 transform;
        int type;
        Image* image;
    };

    const Scene* scene = nullptr;

    std::vector<PerBatchConstantBuffer> m_batch_buffers;

    // @TODO: fix this ugly shit

    // @TODO: save pass item somewhere and use index instead of keeping many copies
    std::array<Pass, MAX_CASCADE_COUNT> shadow_passes;
    FixedStack<Pass, MAX_LIGHT_CAST_SHADOW_COUNT> point_shadow_passes;

    Pass voxel_pass;
    Pass main_pass;

    void update(const Scene* p_scene);

private:
    uint32_t find_or_add_batch(ecs::Entity p_entity, const PerBatchConstantBuffer& p_buffer);

    void clear();

    void point_light_draw_data();
    void fill(const Scene* p_scene, Pass& p_pass, FilterObjectFunc1 p_func1, FilterObjectFunc2 p_func2);

    std::unordered_map<ecs::Entity, uint32_t> m_batch_buffer_lookup;
};

}  // namespace my