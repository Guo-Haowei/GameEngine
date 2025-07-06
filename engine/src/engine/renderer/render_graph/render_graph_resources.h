#ifndef RG_RESOURCE
#define RG_RESOURCE(...)

#ifndef INCLUDE_AS_HEADER
#define INCLUDE_AS_HEADER
#include "engine/renderer/gpu_resource.h"

enum : int {
    RESOURCE_SIZE_FRAME = -1,
    RESOURCE_SIZE_INFI_SHADOW = -2,
    RESOURCE_SIZE_POINT_SHADOW = -3,
};

namespace my {
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

#endif  // !INCLUDE_AS_HEADER

#endif  // !RG_RESOURCE

#if !USING(PLATFORM_WASM)

RG_RESOURCE(RESOURCE_ENV_SKYBOX_CUBE_MAP,
            CubemapSampler(),
            PixelFormat::R32G32B32A32_FLOAT,
            AttachmentType::COLOR_CUBE,
            RT_SIZE_IBL_CUBEMAP,
            RT_SIZE_IBL_CUBEMAP,
            6,
            RESOURCE_MISC_GENERATE_MIPS,
            IBL_MIP_CHAIN_MAX)

RG_RESOURCE(RESOURCE_ENV_DIFFUSE_IRRADIANCE_CUBE_MAP,
            CubemapNoMipSampler(),
            PixelFormat::R32G32B32A32_FLOAT,
            AttachmentType::COLOR_CUBE,
            RT_SIZE_IBL_IRRADIANCE_CUBEMAP,
            RT_SIZE_IBL_IRRADIANCE_CUBEMAP,
            6)

RG_RESOURCE(RESOURCE_ENV_PREFILTER_CUBE_MAP,
            CubemapLodSampler(),
            PixelFormat::R32G32B32A32_FLOAT,
            AttachmentType::COLOR_CUBE,
            RT_SIZE_IBL_PREFILTERED_CUBEMAP,
            RT_SIZE_IBL_PREFILTERED_CUBEMAP,
            6,
            RESOURCE_MISC_GENERATE_MIPS,
            IBL_MIP_CHAIN_MAX)

#undef RG_RESOURCE

#endif
