#pragma once
#if !defined(INCLUDE_DX11) && !defined(INCLUDE_DX12)
#define INCLUDE_DX11
#endif

#ifdef DX
#undef DX
#endif

#if defined(INCLUDE_DX11)
#include <d3d11.h>
#define D3D_(a)                      D3D11_##a
#define D3D_INPUT_CLASSIFICATION_(a) D3D11_INPUT_##a
#define D3D_FILL_MODE_(a)            D3D11_FILL_##a
#define D3D_CULL_MODE_(a)            D3D11_CULL_##a
#define D3D_COMPARISON_(a)           D3D11_COMPARISON_##a
#define D3D_TEXTURE_ADDRESS_MODE_(a) D3D11_TEXTURE_ADDRESS_##a
#elif defined(INCLUDE_DX12)
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

using INPUT_CLASSIFICATION = D3D_(INPUT_CLASSIFICATION);
using FILL_MODE = D3D_(FILL_MODE);
using CULL_MODE = D3D_(CULL_MODE);
using DEPTH_WRITE_MASK = D3D_(DEPTH_WRITE_MASK);
using COMPARISON_FUNC = D3D_(COMPARISON_FUNC);
using FILTER = D3D_(FILTER);
using TEXTURE_ADDRESS_MODE = D3D_(TEXTURE_ADDRESS_MODE);

#include "Core/Enums.h"

static inline DXGI_FORMAT Convert(Format format)
{
    switch (format)
    {
        case Format::R8G8B8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case Format::R32G32_FLOAT:
            return DXGI_FORMAT_R32G32_FLOAT;
        case Format::R32G32B32_FLOAT:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case Format::R32G32B32A32_FLOAT:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case Format::R32G32_SINT:
            return DXGI_FORMAT_R32G32_SINT;
        case Format::R32G32B32_SINT:
            return DXGI_FORMAT_R32G32B32_SINT;
        case Format::R32G32B32A32_SINT:
            return DXGI_FORMAT_R32G32B32A32_SINT;
    }
    return DXGI_FORMAT_UNKNOWN;
}

static inline INPUT_CLASSIFICATION Convert(InputClassification input_classification)
{
    switch (input_classification)
    {
        case InputClassification::PER_VERTEX_DATA:
            return D3D_INPUT_CLASSIFICATION_(PER_VERTEX_DATA);
        case InputClassification::PER_INSTANCE_DATA:
            return D3D_INPUT_CLASSIFICATION_(PER_INSTANCE_DATA);
    }
    return D3D_INPUT_CLASSIFICATION_(PER_VERTEX_DATA);
}

static inline FILL_MODE Convert(FillMode fill_mode)
{
    switch (fill_mode)
    {
        case FillMode::SOLID:
            return D3D_FILL_MODE_(SOLID);
        case FillMode::WIREFRAME:
            return D3D_FILL_MODE_(WIREFRAME);
    }
    return D3D_FILL_MODE_(SOLID);
}

static inline CULL_MODE Convert(CullMode cull_mode)
{
    switch (cull_mode)
    {
        case CullMode::NONE:
            return D3D_CULL_MODE_(NONE);
        case CullMode::FRONT:
            return D3D_CULL_MODE_(FRONT);
        case CullMode::BACK:
            return D3D_CULL_MODE_(BACK);
    }
    return D3D_CULL_MODE_(NONE);
}

static inline DEPTH_WRITE_MASK Convert(DepthWriteMask depth_write_mask)
{
    switch (depth_write_mask)
    {
        case DepthWriteMask::ZERO:
            return D3D_(DEPTH_WRITE_MASK_ZERO);
        case DepthWriteMask::ALL:
            return D3D_(DEPTH_WRITE_MASK_ALL);
    }
    return D3D_(DEPTH_WRITE_MASK_ZERO);
}

static inline COMPARISON_FUNC Convert(ComparisonFunc comparison_func)
{
    switch (comparison_func)
    {
        case ComparisonFunc::NEVER:
            return D3D_COMPARISON_(NEVER);
        case ComparisonFunc::LESS:
            return D3D_COMPARISON_(LESS);
        case ComparisonFunc::EQUAL:
            return D3D_COMPARISON_(EQUAL);
        case ComparisonFunc::LESS_EQUAL:
            return D3D_COMPARISON_(LESS_EQUAL);
        case ComparisonFunc::GREATER:
            return D3D_COMPARISON_(GREATER);
        case ComparisonFunc::GREATER_EQUAL:
            return D3D_COMPARISON_(GREATER_EQUAL);
        case ComparisonFunc::ALWAYS:
            return D3D_COMPARISON_(ALWAYS);
    }
    return D3D_COMPARISON_(NEVER);
}

static inline FILTER Convert(Filter filter)
{
    switch (filter)
    {
        case Filter::MIN_MAG_MIP_POINT:
            return D3D_(FILTER_MIN_MAG_MIP_POINT);
        case Filter::MIN_MAG_MIP_LINEAR:
            return D3D_(FILTER_MIN_MAG_MIP_LINEAR);
        case Filter::ANISOTROPIC:
            return D3D_(FILTER_ANISOTROPIC);
    }
    return D3D_(FILTER_MIN_MAG_MIP_POINT);
}

static inline TEXTURE_ADDRESS_MODE Convert(TextureAddressMode texture_address_mode)
{
    switch (texture_address_mode)
    {
        case TextureAddressMode::WRAP:
            return D3D_TEXTURE_ADDRESS_MODE_(WRAP);
        case TextureAddressMode::MIRROR:
            return D3D_TEXTURE_ADDRESS_MODE_(MIRROR);
        case TextureAddressMode::CLAMP:
            return D3D_TEXTURE_ADDRESS_MODE_(CLAMP);
        case TextureAddressMode::BORDER:
            return D3D_TEXTURE_ADDRESS_MODE_(BORDER);
        case TextureAddressMode::MIRROR_ONCE:
            return D3D_TEXTURE_ADDRESS_MODE_(MIRROR_ONCE);
    }
    return D3D_TEXTURE_ADDRESS_MODE_(WRAP);
}

#undef D3D
#undef D3D_INPUT_CLASSIFICATION
#undef D3D_FILL_MODE
#undef D3D_CULL_MODE
#undef D3D_COMPARISON_FUNC
#undef D3D_TEXTURE_ADDRESS_MODE