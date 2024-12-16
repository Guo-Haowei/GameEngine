#ifdef CONVERT_H_INCLUDED
#error DO NOT INCLUDE THIS IN A HEADER FILE
#endif
#define CONVERT_H_INCLUDED

#if !defined(INCLUDE_AS_D3D11) && !defined(INCLUDE_AS_D3D12)
#define INCLUDE_AS_D3D11
#endif  // !INCLUDE_AS_D3D11) && !defined(INCLUDE_AS_D3D12)

// @TODO: move to a common place
#if defined(INCLUDE_AS_D3D11)
#include <d3d11.h>
#define D3D_(a)                      D3D11_##a
#define D3D_INPUT_CLASSIFICATION_(a) D3D11_INPUT_##a
#define D3D_FILL_MODE_(a)            D3D11_FILL_##a
#define D3D_CULL_MODE_(a)            D3D11_CULL_##a
#define D3D_COMPARISON_(a)           D3D11_COMPARISON_##a
#define D3D_FILTER_(a)               D3D11_FILTER_##a
#define D3D_TEXTURE_ADDRESS_MODE_(a) D3D11_TEXTURE_ADDRESS_##a
#define D3D_STENCIL_OP_(a)           D3D11_STENCIL_OP_##a
#define D3D_BLEND_(a)                D3D11_BLEND_##a
#elif defined(INCLUDE_AS_D3D12)
#include <d3d12.h>
#define D3D_(a)                      D3D12_##a
#define D3D_INPUT_CLASSIFICATION_(a) D3D12_INPUT_CLASSIFICATION_##a
#define D3D_FILL_MODE_(a)            D3D12_FILL_MODE_##a
#define D3D_CULL_MODE_(a)            D3D12_CULL_MODE_##a
#define D3D_COMPARISON_(a)           D3D12_COMPARISON_FUNC_##a
#define D3D_FILTER_(a)               D3D12_FILTER_##a
#define D3D_TEXTURE_ADDRESS_MODE_(a) D3D12_TEXTURE_ADDRESS_MODE_##a
#define D3D_STENCIL_OP_(a)           D3D12_STENCIL_OP_##a
#define D3D_BLEND_(a)                D3D12_BLEND_##a
#else
#error "Unknown API"
#endif

#include "engine/renderer/pipeline_state.h"
#include "engine/renderer/pixel_format.h"

namespace my::d3d {

static inline DXGI_FORMAT Convert(PixelFormat p_format) {
    switch (p_format) {
        // @TODO: use macro
        case PixelFormat::UNKNOWN:
            return DXGI_FORMAT_UNKNOWN;
        case PixelFormat::R8_UINT:
            return DXGI_FORMAT_R8_UNORM;
        case PixelFormat::R8G8_UINT:
            return DXGI_FORMAT_R8G8_UNORM;
        case PixelFormat::R8G8B8_UINT:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::R8G8B8A8_UINT:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case PixelFormat::R16_FLOAT:
            return DXGI_FORMAT_R16_FLOAT;
        case PixelFormat::R16G16_FLOAT:
            return DXGI_FORMAT_R16G16_FLOAT;
        case PixelFormat::R16G16B16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case PixelFormat::R16G16B16A16_FLOAT:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case PixelFormat::R32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case PixelFormat::R32G32_FLOAT:
            return DXGI_FORMAT_R32G32_FLOAT;
        case PixelFormat::R32G32B32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case PixelFormat::R32G32B32A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case PixelFormat::R32G32_SINT:
            return DXGI_FORMAT_R32G32_SINT;
        case PixelFormat::R32G32B32_SINT:
            return DXGI_FORMAT_R32G32B32_SINT;
        case PixelFormat::R32G32B32A32_SINT:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case PixelFormat::R11G11B10_FLOAT:
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case PixelFormat::D32_FLOAT:
            return DXGI_FORMAT_D32_FLOAT;
        case PixelFormat::R24G8_TYPELESS:
            return DXGI_FORMAT_R24G8_TYPELESS;
        case PixelFormat::R24_UNORM_X8_TYPELESS:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case PixelFormat::D24_UNORM_S8_UINT:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        case PixelFormat::X24_TYPELESS_G8_UINT:
            return DXGI_FORMAT_X24_TYPELESS_G8_UINT;
        case PixelFormat::R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        default:
            CRASH_NOW();
            return DXGI_FORMAT_UNKNOWN;
    }
}

static inline D3D_(INPUT_CLASSIFICATION) Convert(InputClassification p_input_classification) {
    switch (p_input_classification) {
        case InputClassification::PER_VERTEX_DATA:
            return D3D_INPUT_CLASSIFICATION_(PER_VERTEX_DATA);
        case InputClassification::PER_INSTANCE_DATA:
            return D3D_INPUT_CLASSIFICATION_(PER_INSTANCE_DATA);
    }
    return D3D_INPUT_CLASSIFICATION_(PER_VERTEX_DATA);
}

static inline D3D_(FILL_MODE) Convert(FillMode p_fill_mode) {
    switch (p_fill_mode) {
        case FillMode::SOLID:
            return D3D_FILL_MODE_(SOLID);
        case FillMode::WIREFRAME:
            return D3D_FILL_MODE_(WIREFRAME);
    }
    return D3D_FILL_MODE_(SOLID);
}

static inline D3D_(CULL_MODE) Convert(CullMode p_cull_mode) {
    switch (p_cull_mode) {
        case CullMode::NONE:
            return D3D_CULL_MODE_(NONE);
        case CullMode::FRONT:
            return D3D_CULL_MODE_(FRONT);
        case CullMode::BACK:
            return D3D_CULL_MODE_(BACK);
    }
    return D3D_CULL_MODE_(NONE);
}

static inline D3D_(COMPARISON_FUNC) Convert(ComparisonFunc p_func) {
    switch (p_func) {
#define COMPARISON_FUNC_ENUM(ENUM, VALUE) \
    case ComparisonFunc::ENUM:            \
        return D3D_COMPARISON_(ENUM);
        COMPARISON_FUNC_ENUM_LIST
#undef COMPARISON_FUNC_ENUM
        default:
            CRASH_NOW();
            return D3D_COMPARISON_(NEVER);
    }
}

static inline D3D_(TEXTURE_ADDRESS_MODE) Convert(AddressMode p_mode) {
    switch (p_mode) {
        case AddressMode::WRAP:
            return D3D_TEXTURE_ADDRESS_MODE_(WRAP);
        case AddressMode::CLAMP:
            return D3D_TEXTURE_ADDRESS_MODE_(CLAMP);
        case AddressMode::BORDER:
            return D3D_TEXTURE_ADDRESS_MODE_(BORDER);
        default:
            CRASH_NOW();
            return D3D_TEXTURE_ADDRESS_MODE_(WRAP);
    }
}

static inline D3D_(BLEND) Convert(Blend p_blend) {
    switch (p_blend) {
        case Blend::BLEND_ZERO:
            return D3D_BLEND_(ZERO);
        case Blend::BLEND_ONE:
            return D3D_BLEND_(ONE);
        case Blend::BLEND_SRC_ALPHA:
            return D3D_BLEND_(SRC_ALPHA);
        case Blend::BLEND_INV_SRC_ALPHA:
            return D3D_BLEND_(INV_SRC_ALPHA);
        default:
            CRASH_NOW();
            return D3D_BLEND_(ZERO);
    }
}

static inline D3D_(BLEND_OP) Convert(BlendOp p_blend_op) {
    switch (p_blend_op) {
        case BlendOp::BLEND_OP_ADD:
            return D3D_BLEND_(OP_ADD);
        case BlendOp::BLEND_OP_SUB:
            return D3D_BLEND_(OP_SUBTRACT);
        default:
            CRASH_NOW();
            return D3D_BLEND_(OP_ADD);
    }
}

static inline D3D_(STENCIL_OP) Convert(StencilOp p_op) {
    switch (p_op) {
#define STENCIL_OP_ENUM(ENUM, VALUE) \
    case StencilOp::ENUM:            \
        return D3D_STENCIL_OP_(ENUM);
        STENCIL_OP_ENUM_LIST
#undef STENCIL_OP_ENUM
        default:
            CRASH_NOW();
            return D3D_STENCIL_OP_(KEEP);
    }
}

// @TODO: refactor this
static inline D3D_(FILTER) Convert(FilterMode p_min_filter, FilterMode p_mag_filter) {
    if (p_min_filter == FilterMode::MIPMAP_LINEAR && p_mag_filter == FilterMode::MIPMAP_LINEAR) {
        return D3D_FILTER_(MIN_MAG_MIP_LINEAR);
    }
    if (p_min_filter == FilterMode::POINT && p_mag_filter == FilterMode::POINT) {
        return D3D_FILTER_(MIN_MAG_MIP_POINT);
    }
    if (p_min_filter == FilterMode::LINEAR && p_mag_filter == FilterMode::LINEAR) {
        return D3D_FILTER_(MIN_MAG_LINEAR_MIP_POINT);
    }
    if (p_min_filter == FilterMode::MIPMAP_LINEAR && p_mag_filter == FilterMode::LINEAR) {
        return D3D_FILTER_(MIN_LINEAR_MAG_POINT_MIP_LINEAR);
    }

    CRASH_NOW_MSG(std::format("Unknown filter {} and {}", static_cast<int>(p_min_filter), static_cast<int>(p_mag_filter)));
    return D3D_FILTER_(MIN_MAG_MIP_POINT);
}

static inline auto Convert(const StencilOpDesc* p_in) {
    if (!p_in) {
        CRASH_NOW_MSG("TODO: default");
    }

    D3D_(DEPTH_STENCILOP_DESC)
    desc{};
    desc.StencilFunc = Convert(p_in->stencilFunc);
    desc.StencilFailOp = Convert(p_in->stencilFailOp);
    desc.StencilDepthFailOp = Convert(p_in->stencilDepthFailOp);
    desc.StencilPassOp = Convert(p_in->stencilPassOp);
    return desc;
}

static inline auto Convert(const BlendDesc* p_in) {
    if (!p_in) {
        CRASH_NOW_MSG("TODO: default");
    }

    D3D_(BLEND_DESC)
    desc;
    ZeroMemory(&desc, sizeof(desc));
    for (int i = 0; i < array_length(p_in->renderTargets); ++i) {
        auto& out = desc.RenderTarget[i];
        const auto& in = p_in->renderTargets[i];
        out.BlendEnable = in.blendEnabled;
        out.SrcBlend = out.SrcBlendAlpha = Convert(in.blendSrc);
        out.DestBlend = out.DestBlendAlpha = Convert(in.blendDest);
        out.BlendOp = out.BlendOpAlpha = Convert(in.blendOp);
        out.RenderTargetWriteMask = in.colorWriteMask;
    }
    return desc;
}

static inline auto Convert(const DepthStencilDesc* p_in) {
    if (!p_in) {
        CRASH_NOW_MSG("TODO: default");
    }

    D3D_(DEPTH_STENCIL_DESC)
    desc{};
    desc.DepthEnable = p_in->depthEnabled;
    desc.DepthFunc = d3d::Convert(p_in->depthFunc);
    desc.DepthWriteMask = D3D_(DEPTH_WRITE_MASK_ALL);
    desc.StencilEnable = p_in->stencilEnabled;
    desc.StencilWriteMask = p_in->stencilWriteMask;
    desc.StencilReadMask = p_in->stencilReadMask;

    desc.FrontFace = d3d::Convert(&p_in->frontFace);
    desc.BackFace = d3d::Convert(&p_in->backFace);
    return desc;
}

#if defined(INCLUDE_AS_D3D11)
static inline D3D11_USAGE Convert(BufferUsage p_usage) {
    switch (p_usage) {
        case BufferUsage::IMMUTABLE:
            return D3D11_USAGE_IMMUTABLE;
        case BufferUsage::DYNAMIC:
            return D3D11_USAGE_DYNAMIC;
        case BufferUsage::STAGING:
            return D3D11_USAGE_STAGING;
        case BufferUsage::DEFAULT:
            [[fallthrough]];
        default:
            return D3D11_USAGE_DEFAULT;
    }
}
#endif

#if defined(INCLUDE_AS_D3D12)
static inline D3D12_STATIC_BORDER_COLOR Convert(StaticBorderColor p_color) {
    switch (p_color) {
        case StaticBorderColor::TRANSPARENT_BLACK:
            return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        case StaticBorderColor::OPAQUE_BLACK:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        case StaticBorderColor::OPAQUE_WHITE:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        default:
            CRASH_NOW();
            return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    }
}
#endif

static inline D3D_PRIMITIVE_TOPOLOGY Convert(PrimitiveTopology p_topology) {
    switch (p_topology) {
        case PrimitiveTopology::POINT:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case PrimitiveTopology::LINE:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case PrimitiveTopology::TRIANGLE:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        default:
            CRASH_NOW();
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

#if defined(INCLUDE_AS_D3D12)
static inline D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertToType(PrimitiveTopology p_topology) {
    switch (p_topology) {
        case PrimitiveTopology::POINT:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case PrimitiveTopology::LINE:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case PrimitiveTopology::TRIANGLE:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        default:
            CRASH_NOW();
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    }
}
#endif

#undef D3D_
#undef D3D_INPUT_CLASSIFICATION_
#undef D3D_FILL_MODE_
#undef D3D_CULL_MODE_
#undef D3D_COMPARISON_FUNC_
#undef D3D_FILTER_
#undef D3D_TEXTURE_ADDRESS_MODE_
#undef D3D_STENCIL_OP_
#undef D3D_BLEND_

}  // namespace my::d3d
