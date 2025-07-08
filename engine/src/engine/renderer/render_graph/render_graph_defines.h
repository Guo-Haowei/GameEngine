#pragma once
#include "engine/renderer/pixel_format.h"
#include "engine/renderer/render_graph/render_graph_resources.h"

namespace my {

constexpr const char RG_PASS_EMPTY[] = "p:empty";
constexpr const char RG_PASS_EARLY_Z[] = "p:early_z";
constexpr const char RG_PASS_SHADOW[] = "p:shadow";
constexpr const char RG_PASS_GBUFFER[] = "p:gbuffer";
constexpr const char RG_PASS_VOXELIZATION[] = "p:voxelization";
constexpr const char RG_PASS_LIGHTING[] = "p:lighting";
constexpr const char RG_PASS_FORWARD[] = "p:forward";
constexpr const char RG_PASS_BLOOM_SETUP[] = "p:bloom_setup";
constexpr const char RG_PASS_POST_PROCESS[] = "p:post_process";
constexpr const char RG_PASS_OVERLAY[] = "p:overlay";
constexpr const char RG_PASS_SSAO[] = "p:ssao";
constexpr const char RG_PASS_OUTLINE[] = "p:outline";
constexpr const char RG_PASS_PATHTRACER[] = "p:pathtracer";
constexpr const char RG_PASS_PATHTRACER_PRESENT[] = "p:pathtracer_present";

constexpr const char RG_RES_DEPTH_STENCIL[] = "r:depth";
constexpr const char RG_RES_SHADOW_MAP[] = "r:shadow";
constexpr const char RG_RES_GBUFFER_COLOR0[] = "r:gbuffer0";
constexpr const char RG_RES_GBUFFER_COLOR1[] = "r:gbuffer1";
constexpr const char RG_RES_GBUFFER_COLOR2[] = "r:gbuffer2";
constexpr const char RG_RES_SSAO[] = "r:ssao";
constexpr const char RG_RES_SSAO_NOISE[] = "r:ssao_noise";
constexpr const char RG_RES_LIGHTING[] = "r:lighting";
constexpr const char RG_RES_POST_PROCESS[] = "r:post_process";
constexpr const char RG_RES_OVERLAY[] = "r:overlay";
constexpr const char RG_RES_VOXEL_LIGHTING[] = "r:voxel_lighting";
constexpr const char RG_RES_VOXEL_NORMAL[] = "r:voxel_normal";
constexpr const char RG_RES_OUTLINE[] = "r:outline";
constexpr const char RG_RES_PATHTRACER[] = "r:pathtracer";

#define RG_PASS_BLOOM_DOWN_PREFIX "p:bloom_downsample_"
#define RG_PASS_BLOOM_UP_PREFIX   "p:bloom_upsample_"
#define RG_RES_BLOOM_PREFIX       "r:bloom_"

}  // namespace my
