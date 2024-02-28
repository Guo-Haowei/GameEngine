#include "r_cbuffers.h"

#include "core/framework/graphics_manager.h"

namespace my {

UniformBuffer<PerBatchConstantBuffer> g_per_batch_uniform;
UniformBuffer<PerPassConstantBuffer> g_per_pass_cache;
UniformBuffer<PerFrameConstantBuffer> g_perFrameCache;
UniformBuffer<MaterialConstantBuffer> g_materialCache;
UniformBuffer<PerSceneConstantBuffer> g_constantCache;
UniformBuffer<BoneConstantBuffer> g_boneCache;
UniformBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;

template<typename T>
static void create_uniform_buffer(UniformBuffer<T>& p_buffer) {
    constexpr int slot = T::get_uniform_buffer_slot();
    p_buffer.buffer = GraphicsManager::singleton().create_uniform_buffer(slot, sizeof(T));
}

}  // namespace my

void R_Alloc_Cbuffers() {
    using namespace my;
    create_uniform_buffer<PerBatchConstantBuffer>(g_per_batch_uniform);
    create_uniform_buffer<PerPassConstantBuffer>(g_per_pass_cache);
    create_uniform_buffer<PerFrameConstantBuffer>(g_perFrameCache);
    create_uniform_buffer<MaterialConstantBuffer>(g_materialCache);
    create_uniform_buffer<PerSceneConstantBuffer>(g_constantCache);
    create_uniform_buffer<BoneConstantBuffer>(g_boneCache);
    create_uniform_buffer<DebugDrawConstantBuffer>(g_debug_draw_cache);
}
