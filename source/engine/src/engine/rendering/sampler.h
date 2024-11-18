#pragma once
#include "rendering/graphics_defines.h"

namespace my {

struct SamplerDesc {
    SamplerDesc(FilterMode p_min_filter = FilterMode::POINT,
                FilterMode p_mag_filter = FilterMode::POINT,
                AddressMode p_address_mode = AddressMode::WRAP,
                StaticBorderColor p_border_color = StaticBorderColor::TRANSPARENT_BLACK,
                float p_min_lod_bias = 0.0f,
                uint32_t p_max_anisotropy = 0.0f,
                ComparisonFunc p_func = ComparisonFunc::LESS_EQUAL,
                float p_min_lod = 0.0f,
                float p_max_lod = 3.402823466e+38f) {
        minFilter = p_min_filter;
        magFilter = p_mag_filter;
        addressU = p_address_mode;
        addressV = p_address_mode;
        addressW = p_address_mode;
        staticBorderColor = p_border_color;
        mipLodBias = p_min_lod_bias;
        maxAnisotropy = p_max_anisotropy;
        comparisonFunc = p_func;
        minLod = p_min_lod;
        maxLod = p_max_lod;

        switch (p_border_color) {
            case my::StaticBorderColor::OPAQUE_BLACK:
                border[0] = 0.0f;
                border[1] = 0.0f;
                border[2] = 0.0f;
                border[3] = 1.0f;
                break;
            case my::StaticBorderColor::OPAQUE_WHITE:
                border[0] = 1.0f;
                border[1] = 1.0f;
                border[2] = 1.0f;
                border[3] = 1.0f;
                break;
            case my::StaticBorderColor::TRANSPARENT_BLACK:
                [[fallthrough]];
            default:
                border[0] = 0.0f;
                border[1] = 0.0f;
                border[2] = 0.0f;
                border[3] = 0.0f;
                break;
        }
    }

    // @TODO: mipmap filter
    FilterMode minFilter;
    FilterMode magFilter;

    AddressMode addressU;
    AddressMode addressV;
    AddressMode addressW;

    StaticBorderColor staticBorderColor;

    float mipLodBias;
    uint32_t maxAnisotropy;
    ComparisonFunc comparisonFunc;
    float border[4];
    float minLod;
    float maxLod;
};

static inline SamplerDesc PointClampSampler() {
    SamplerDesc desc(FilterMode::POINT, FilterMode::POINT, AddressMode::CLAMP);
    return desc;
}

static inline SamplerDesc LinearClampSampler() {
    SamplerDesc desc(FilterMode::LINEAR, FilterMode::LINEAR, AddressMode::CLAMP);
    return desc;
}

// @TODO: refactor
inline SamplerDesc bloom_downsample() {
    SamplerDesc desc{};
    desc.minFilter = desc.magFilter = FilterMode::LINEAR;
    desc.addressU = desc.addressV = desc.addressW = AddressMode::BORDER;
    desc.border[0] = 0.0;
    desc.border[1] = 0.0;
    desc.border[2] = 0.0;
    desc.border[3] = 1.0f;
    return desc;
}

inline SamplerDesc env_cube_map_sampler_mip() {
    SamplerDesc desc{};
    desc.minFilter = FilterMode::MIPMAP_LINEAR;
    desc.magFilter = FilterMode::LINEAR;
    desc.addressU = desc.addressV = desc.addressW = AddressMode::CLAMP;
    return desc;
}

inline SamplerDesc shadow_cube_map_sampler() {
    SamplerDesc desc{};
    desc.minFilter = desc.magFilter = FilterMode::POINT;
    desc.addressU = desc.addressV = desc.addressW = AddressMode::CLAMP;
    return desc;
}

inline SamplerDesc shadow_map_sampler() {
    SamplerDesc desc{};
    desc.minFilter = desc.magFilter = FilterMode::POINT;
    desc.addressU = desc.addressV = desc.addressW = AddressMode::BORDER;
    desc.border[0] = 1.0f;
    desc.border[1] = 1.0f;
    desc.border[2] = 1.0f;
    desc.border[3] = 1.0f;
    return desc;
}

}  // namespace my
