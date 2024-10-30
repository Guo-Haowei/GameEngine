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
#define D3D(a)                      D3D11_##a
#define D3D_INPUT_CLASSIFICATION(a) D3D11_INPUT_##a
#define D3D_FILL_MODE(a)            D3D11_FILL_##a
#define D3D_CULL_MODE(a)            D3D11_CULL_##a
#define D3D_COMPARISON(a)           D3D11_COMPARISON_##a
#define D3D_TEXTURE_ADDRESS_MODE(a) D3D11_TEXTURE_ADDRESS_##a
#define D3D_USAGE(a)                D3D11_USAGE_##a
#elif defined(INCLUDE_AS_D3D12)
#include <d3d12.h>
#define D3D(a)                      D3D12_##a
#define D3D_INPUT_CLASSIFICATION(a) D3D12_INPUT_CLASSIFICATION_##a
#define D3D_FILL_MODE(a)            D3D12_FILL_MODE_##a
#define D3D_CULL_MODE(a)            D3D12_CULL_MODE_##a
#define D3D_COMPARISON(a)           D3D12_COMPARISON_FUNC_##a
#define D3D_TEXTURE_ADDRESS_MODE(a) D3D12_TEXTURE_ADDRESS_MODE_##a
#else
#error "Unknown API"
#endif

#include "rendering/pipeline_state.h"
#include "rendering/pixel_format.h"

namespace my::d3d {

using D3D_INPUT_CLASSIFICATION = D3D(INPUT_CLASSIFICATION);
using D3D_FILL_MODE = D3D(FILL_MODE);
using D3D_CULL_MODE = D3D(CULL_MODE);
using D3D_DEPTH_WRITE_MASK = D3D(DEPTH_WRITE_MASK);
using D3D_COMPARISON_FUNC = D3D(COMPARISON_FUNC);
using D3D_FILTER = D3D(FILTER);
using D3D_TEXTURE_ADDRESS_MODE = D3D(TEXTURE_ADDRESS_MODE);
using D3D_USAGE = D3D(USAGE);

static inline DXGI_FORMAT Convert(PixelFormat p_format) {
    switch (p_format) {
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
        case PixelFormat::D24_UNORM_S8_UINT:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;
        default:
            CRASH_NOW();
            return DXGI_FORMAT_UNKNOWN;
    }
}

static inline D3D_INPUT_CLASSIFICATION Convert(InputClassification p_input_classification) {
    switch (p_input_classification) {
        case InputClassification::PER_VERTEX_DATA:
            return D3D_INPUT_CLASSIFICATION(PER_VERTEX_DATA);
        case InputClassification::PER_INSTANCE_DATA:
            return D3D_INPUT_CLASSIFICATION(PER_INSTANCE_DATA);
    }
    return D3D_INPUT_CLASSIFICATION(PER_VERTEX_DATA);
}

static inline D3D_FILL_MODE Convert(FillMode p_fill_mode) {
    switch (p_fill_mode) {
        case FillMode::SOLID:
            return D3D_FILL_MODE(SOLID);
        case FillMode::WIREFRAME:
            return D3D_FILL_MODE(WIREFRAME);
    }
    return D3D_FILL_MODE(SOLID);
}

static inline D3D_CULL_MODE Convert(CullMode p_cull_mode) {
    switch (p_cull_mode) {
        case CullMode::NONE:
            return D3D_CULL_MODE(NONE);
        case CullMode::FRONT:
            return D3D_CULL_MODE(FRONT);
        case CullMode::BACK:
            return D3D_CULL_MODE(BACK);
    }
    return D3D_CULL_MODE(NONE);
}

static inline D3D_COMPARISON_FUNC Convert(ComparisonFunc p_func) {
    switch (p_func) {
        case ComparisonFunc::NEVER:
            return D3D_COMPARISON(NEVER);
        case ComparisonFunc::LESS:
            return D3D_COMPARISON(LESS);
        case ComparisonFunc::EQUAL:
            return D3D_COMPARISON(EQUAL);
        case ComparisonFunc::LESS_EQUAL:
            return D3D_COMPARISON(LESS_EQUAL);
        case ComparisonFunc::GREATER:
            return D3D_COMPARISON(GREATER);
        case ComparisonFunc::GREATER_EQUAL:
            return D3D_COMPARISON(GREATER_EQUAL);
        case ComparisonFunc::ALWAYS:
            return D3D_COMPARISON(ALWAYS);
    }
    return D3D_COMPARISON(NEVER);
}

static inline D3D_USAGE Convert(BufferUsage p_usage) {
    switch (p_usage) {
        case my::BufferUsage::DEFAULT:
            return D3D_USAGE(DEFAULT);
        case my::BufferUsage::IMMUTABLE:
            return D3D_USAGE(IMMUTABLE);
        case my::BufferUsage::DYNAMIC:
            return D3D_USAGE(DYNAMIC);
        case my::BufferUsage::STAGING:
            return D3D_USAGE(STAGING);
    }
    return D3D_USAGE(DEFAULT);
}

#if 0
static inline DEPTH_WRITE_MASK Convert(DepthWriteMask depth_write_mask) {
    switch (depth_write_mask) {
        case DepthWriteMask::ZERO:
            return D3D(DEPTH_WRITE_MASK_ZERO);
        case DepthWriteMask::ALL:
            return D3D(DEPTH_WRITE_MASK_ALL);
    }
    return D3D(DEPTH_WRITE_MASK_ZERO);
}

static inline FILTER Convert(Filter filter) {
    switch (filter) {
        case Filter::MIN_MAG_MIP_POINT:
            return D3D(FILTER_MIN_MAG_MIP_POINT);
        case Filter::MIN_MAG_MIP_LINEAR:
            return D3D(FILTER_MIN_MAG_MIP_LINEAR);
        case Filter::ANISOTROPIC:
            return D3D(FILTER_ANISOTROPIC);
    }
    return D3D(FILTER_MIN_MAG_MIP_POINT);
}

static inline TEXTURE_ADDRESS_MODE Convert(TextureAddressMode texture_address_mode) {
    switch (texture_address_mode) {
        case TextureAddressMode::WRAP:
            return D3D_TEXTURE_ADDRESS_MODE(WRAP);
        case TextureAddressMode::MIRROR:
            return D3D_TEXTURE_ADDRESS_MODE(MIRROR);
        case TextureAddressMode::CLAMP:
            return D3D_TEXTURE_ADDRESS_MODE(CLAMP);
        case TextureAddressMode::BORDER:
            return D3D_TEXTURE_ADDRESS_MODE(BORDER);
        case TextureAddressMode::MIRROR_ONCE:
            return D3D_TEXTURE_ADDRESS_MODE(MIRROR_ONCE);
    }
    return D3D_TEXTURE_ADDRESS_MODE(WRAP);
}
#endif

#undef D3D
#undef D3D_INPUT_CLASSIFICATION
#undef D3D_FILL_MODE
#undef D3D_CULL_MODE
#undef D3D_COMPARISON_FUNC
#undef D3D_TEXTURE_ADDRESS_MODE
#undef D3D_USAGE

}  // namespace my::d3d
