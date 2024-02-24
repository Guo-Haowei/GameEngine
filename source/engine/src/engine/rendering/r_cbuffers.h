#pragma once
#include "gl_utils.h"

// inline include
#include "cbuffer.glsl.h"

extern gl::ConstantBuffer<PerBatchConstantBuffer> g_perBatchCache;
extern gl::ConstantBuffer<PerPassConstantBuffer> g_per_pass_cache;
extern gl::ConstantBuffer<PerFrameConstantBuffer> g_perFrameCache;
extern gl::ConstantBuffer<MaterialConstantBuffer> g_materialCache;
extern gl::ConstantBuffer<PerSceneConstantBuffer> g_constantCache;
extern gl::ConstantBuffer<BoneConstantBuffer> g_boneCache;
extern gl::ConstantBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;

void R_Alloc_Cbuffers();
void R_Destroy_Cbuffers();