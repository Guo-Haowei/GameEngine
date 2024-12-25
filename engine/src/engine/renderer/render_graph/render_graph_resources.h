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
// RT_FMT stands form RENDER_TARGET_FORMAT
constexpr PixelFormat RT_FMT_GBUFFER_DEPTH = PixelFormat::R24G8_TYPELESS;
constexpr PixelFormat RT_FMT_GBUFFER_BASE_COLOR = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_GBUFFER_POSITION = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_GBUFFER_NORMAL = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_GBUFFER_MATERIAL = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_TONE = PixelFormat::R16G16B16A16_FLOAT;
constexpr PixelFormat RT_FMT_OUTLINE_SELECT = PixelFormat::R8_UINT;
// @TODO: rename
constexpr PixelFormat DEFAULT_SURFACE_FORMAT = PixelFormat::R8G8B8A8_UNORM;
constexpr PixelFormat DEFAULT_DEPTH_STENCIL_FORMAT = PixelFormat::D32_FLOAT;
}  // namespace my

#endif  // !INCLUDE_AS_HEADER

#endif  // !RG_RESOURCE

RG_RESOURCE(RESOURCE_GBUFFER_DEPTH,
            PointClampSampler(),
            RT_FMT_GBUFFER_DEPTH,
            AttachmentType::DEPTH_STENCIL_2D,
            RESOURCE_SIZE_FRAME,
            RESOURCE_SIZE_FRAME)

RG_RESOURCE(RESOURCE_GBUFFER_BASE_COLOR,
            PointClampSampler(),
            RT_FMT_GBUFFER_BASE_COLOR,
            AttachmentType::COLOR_2D,
            RESOURCE_SIZE_FRAME,
            RESOURCE_SIZE_FRAME)

RG_RESOURCE(RESOURCE_GBUFFER_POSITION,
            PointClampSampler(),
            RT_FMT_GBUFFER_POSITION,
            AttachmentType::COLOR_2D,
            RESOURCE_SIZE_FRAME,
            RESOURCE_SIZE_FRAME)

RG_RESOURCE(RESOURCE_GBUFFER_NORMAL,
            PointClampSampler(),
            RT_FMT_GBUFFER_NORMAL,
            AttachmentType::COLOR_2D,
            RESOURCE_SIZE_FRAME,
            RESOURCE_SIZE_FRAME)

RG_RESOURCE(RESOURCE_GBUFFER_MATERIAL,
            PointClampSampler(),
            RT_FMT_GBUFFER_MATERIAL,
            AttachmentType::COLOR_2D,
            RESOURCE_SIZE_FRAME,
            RESOURCE_SIZE_FRAME)

RG_RESOURCE(RESOURCE_TONE,
            PointClampSampler(),
            RT_FMT_TONE,
            AttachmentType::COLOR_2D,
            RESOURCE_SIZE_FRAME,
            RESOURCE_SIZE_FRAME)

RG_RESOURCE(RESOURCE_OUTLINE_SELECT,
            PointClampSampler(),
            RT_FMT_OUTLINE_SELECT,
            AttachmentType::COLOR_2D,
            RESOURCE_SIZE_FRAME,
            RESOURCE_SIZE_FRAME)

RG_RESOURCE(RESOURCE_FINAL,
            PointClampSampler(),
            DEFAULT_SURFACE_FORMAT,
            AttachmentType::COLOR_2D,
            RESOURCE_SIZE_FRAME,
            RESOURCE_SIZE_FRAME)

#undef RG_RESOURCE
