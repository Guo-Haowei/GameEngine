#pragma once
#include "engine/renderer/pixel_format.h"

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
constexpr const char RG_PASS_BAKE_SKYBOX[] = "p:env_skybox";
constexpr const char RG_PASS_BAKE_DIFFUSE[] = "p:diffuse";
constexpr const char RG_PASS_BAKE_PREFILTERED[] = "p:prefiltered";

constexpr const char RG_RES_DEPTH_STENCIL[] = "r:depth";
constexpr const char RG_RES_SHADOW_MAP[] = "r:shadow";
constexpr const char RG_RES_GBUFFER_COLOR0[] = "r:gbuffer0";
constexpr const char RG_RES_GBUFFER_COLOR1[] = "r:gbuffer1";
constexpr const char RG_RES_GBUFFER_COLOR2[] = "r:gbuffer2";
constexpr const char RG_RES_SSAO[] = "r:ssao";
constexpr const char RG_RES_LIGHTING[] = "r:lighting";
constexpr const char RG_RES_POST_PROCESS[] = "r:post_process";
constexpr const char RG_RES_OVERLAY[] = "r:overlay";
constexpr const char RG_RES_VOXEL_LIGHTING[] = "r:voxel_lighting";
constexpr const char RG_RES_VOXEL_NORMAL[] = "r:voxel_normal";
constexpr const char RG_RES_OUTLINE[] = "r:outline";
constexpr const char RG_RES_PATHTRACER[] = "r:pathtracer";
constexpr const char RG_RES_ENV_SKYBOX_CUBE[] = "r:env_cube";
constexpr const char RG_RES_ENV_DIFFUSE_CUBE[] = "r:diffuse_cube";
constexpr const char RG_RES_ENV_PREFILTERED_CUBE[] = "r:prefiltered_cube";
// external resources
constexpr const char RG_RES_SSAO_NOISE[] = "r:ssao_noise";
constexpr const char RG_RES_BRDF[] = "r:ssao_brdf";
constexpr const char RG_RES_IBL[] = "r:ibl";

#define RG_PASS_BLOOM_DOWN_PREFIX "p:bloom_downsample_"
#define RG_PASS_BLOOM_UP_PREFIX   "p:bloom_upsample_"
#define RG_RES_BLOOM_PREFIX       "r:bloom_"

constexpr int BLOOM_MIP_CHAIN_MAX = 7;
constexpr int IBL_MIP_CHAIN_MAX = 7;

constexpr int RT_SIZE_IBL_CUBEMAP = 512;
constexpr int RT_SIZE_IBL_IRRADIANCE_CUBEMAP = 32;
constexpr int RT_SIZE_IBL_PREFILTERED_CUBEMAP = 512;
// RT_FMT stands form RENDER_TARGET_FORMAT
constexpr PixelFormat RT_FMT_GBUFFER_DEPTH = PixelFormat::R32G8X24_TYPELESS;
// constexpr PixelFormat RT_FMT_GBUFFER_DEPTH = PixelFormat::R24G8_TYPELESS;
constexpr PixelFormat RT_FMT_GBUFFER_BASE_COLOR = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_GBUFFER_POSITION = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_GBUFFER_NORMAL = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_GBUFFER_MATERIAL = PixelFormat::R16G16B16A16_FLOAT;
// @TODO: debug
constexpr PixelFormat RT_FMT_SSAO = PixelFormat::R32_FLOAT;
constexpr PixelFormat RT_FMT_TONE = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_LIGHTING = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_OUTLINE_SELECT = PixelFormat::R8_UINT;
// @TODO: rename
constexpr PixelFormat DEFAULT_SURFACE_FORMAT = PixelFormat::R8G8B8A8_UNORM;
constexpr PixelFormat DEFAULT_DEPTH_STENCIL_FORMAT = PixelFormat::D32_FLOAT;

}  // namespace my
