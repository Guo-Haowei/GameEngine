#pragma once
#include "core/math/geomath.h"
#include "gl_utils.h"
#include "rendering/uniform_buffer.h"
// include geomath before cbuffer.h
#include "cbuffer.h"

// @TODO: move this to render data
namespace my {

extern UniformBuffer<PerBatchConstantBuffer> g_per_batch_uniform;
extern UniformBuffer<PerPassConstantBuffer> g_per_pass_cache;
extern UniformBuffer<PerFrameConstantBuffer> g_perFrameCache;
extern UniformBuffer<MaterialConstantBuffer> g_materialCache;
extern UniformBuffer<PerSceneConstantBuffer> g_constantCache;
extern UniformBuffer<BoneConstantBuffer> g_boneCache;
extern UniformBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;

}  // namespace my

void R_Alloc_Cbuffers();