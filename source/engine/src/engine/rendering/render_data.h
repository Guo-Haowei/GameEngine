#pragma once
#include "rendering/uniform_buffer.h"
#include "scene/scene.h"

// this should be included after geomath.h
#include "cbuffer.h"

namespace my {

struct MeshBuffers;

extern UniformBuffer<PerPassConstantBuffer> g_per_pass_cache;
extern UniformBuffer<PerFrameConstantBuffer> g_perFrameCache;
extern UniformBuffer<PerSceneConstantBuffer> g_constantCache;
extern UniformBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;

struct RenderData {
    RenderData();

    using FilterObjectFunc1 = std::function<bool(const ObjectComponent& object)>;
    using FilterObjectFunc2 = std::function<bool(const AABB& object_aabb)>;

    struct SubMesh {
        uint32_t index_count;
        uint32_t index_offset;
        int material_idx;
    };

    struct Mesh {
        int bone_idx;
        int batch_idx;
        const MeshBuffers* mesh_data;
        std::vector<SubMesh> subsets;
    };

    struct Pass {
        mat4 projection_matrix;
        mat4 view_matrix;
        mat4 projection_view_matrix;
        LightComponent light_component;

        void fill_perpass(PerPassConstantBuffer& buffer) const;

        std::vector<Mesh> draws;

        void clear() { draws.clear(); }
        bool empty() { return draws.empty(); }
    };

    const Scene* scene = nullptr;

    std::vector<PerBatchConstantBuffer> m_batch_buffers;
    std::vector<MaterialConstantBuffer> m_material_buffers;
    std::vector<BoneConstantBuffer> m_bone_buffers;

    std::shared_ptr<UniformBufferBase> m_batch_uniform;
    std::shared_ptr<UniformBufferBase> m_material_uniform;
    std::shared_ptr<UniformBufferBase> m_bone_uniform;

    // @TODO: fix this ugly shit

    // @TODO: save pass item somewhere and use index instead of keeping many copies
    std::array<Pass, MAX_CASCADE_COUNT> shadow_passes;

    std::array<std::unique_ptr<Pass>, MAX_LIGHT_CAST_SHADOW_COUNT> point_shadow_passes;

    Pass voxel_pass;
    Pass main_pass;

    void update(const Scene* p_scene);

private:
    uint32_t find_or_add_batch(ecs::Entity p_entity, const PerBatchConstantBuffer& p_buffer);
    uint32_t find_or_add_material(ecs::Entity p_entity, const MaterialConstantBuffer& p_buffer);
    uint32_t find_or_add_bone(ecs::Entity p_entity, const BoneConstantBuffer& p_buffer);

    void clear();

    void point_light_draw_data();
    void fill(const Scene* p_scene, Pass& p_pass, FilterObjectFunc1 p_func1, FilterObjectFunc2 p_func2);

    std::unordered_map<ecs::Entity, uint32_t> m_batch_buffer_lookup;
    std::unordered_map<ecs::Entity, uint32_t> m_material_buffer_lookup;
    std::unordered_map<ecs::Entity, uint32_t> m_bone_buffer_lookup;
};

}  // namespace my