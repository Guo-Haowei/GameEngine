#include "r_cbuffers.h"

gl::ConstantBuffer<PerBatchConstantBuffer> g_perBatchCache;
gl::ConstantBuffer<PerPassConstantBuffer> g_per_pass_cache;
gl::ConstantBuffer<PerFrameConstantBuffer> g_perFrameCache;
gl::ConstantBuffer<PerSceneConstantBuffer> g_constantCache;

gl::ConstantBuffer<MaterialConstantBuffer> g_materialCache;
gl::ConstantBuffer<BoneConstantBuffer> g_boneCache;
gl::ConstantBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;

void R_Alloc_Cbuffers() {
    g_perBatchCache.CreateAndBind();
    g_per_pass_cache.CreateAndBind();
    g_perFrameCache.CreateAndBind();
    g_materialCache.CreateAndBind();
    g_constantCache.CreateAndBind();
    g_boneCache.CreateAndBind();
    g_debug_draw_cache.CreateAndBind();
}

void R_Destroy_Cbuffers() {
    g_perBatchCache.Destroy();
    g_per_pass_cache.Destroy();
    g_perFrameCache.Destroy();
    g_materialCache.Destroy();
    g_constantCache.Destroy();
    g_boneCache.Destroy();
    g_debug_draw_cache.Destroy();
}