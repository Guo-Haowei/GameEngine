#pragma once
#include "core/math/geomath.h"
#include "gl_utils.h"
#include "rendering/uniform_buffer.h"
// include geomath before cbuffer.h
#include "hlsl/cbuffer.h"

// @TODO: move this to render data
namespace my {

extern UniformBuffer<PerPassConstantBuffer> g_per_pass_cache;
extern UniformBuffer<PerFrameConstantBuffer> g_perFrameCache;
extern UniformBuffer<PerSceneConstantBuffer> g_constantCache;
extern UniformBuffer<DebugDrawConstantBuffer> g_debug_draw_cache;

}  // namespace my

void R_Alloc_Cbuffers();