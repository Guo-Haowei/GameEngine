#pragma once
#include "rendering/graphics_enum.h"

namespace my {

struct SamplerDesc {
    SamplerDesc() {
        minFilter = FilterMode::POINT;
        magFilter = FilterMode::POINT;
        addressU = AddressMode::WRAP;
        addressV = AddressMode::WRAP;
        addressW = AddressMode::WRAP;
    }

    SamplerDesc(FilterMode p_min_filter, FilterMode p_mag_filter, AddressMode p_address_mode) {
        minFilter = p_min_filter;
        magFilter = p_mag_filter;
        addressU = p_address_mode;
        addressV = p_address_mode;
        addressW = p_address_mode;
    }

    // @TODO: mipmap filter
    FilterMode minFilter;
    FilterMode magFilter;

    AddressMode addressU;
    AddressMode addressV;
    AddressMode addressW;

    float mipLodBias{ 0.0f };
    uint32_t maxAnisotropy{ 16 };
    ComparisonFunc comparisonFunc{ ComparisonFunc::LESS_EQUAL };
    float border[4]{ 0.0f, 0.0f, 0.0f, 0.0f };
    float minLod{ 0.0f };
    float maxLod{ 3.402823466e+38f };
};

static inline SamplerDesc PointClampSampler() {
    SamplerDesc desc(FilterMode::POINT, FilterMode::POINT, AddressMode::CLAMP);
    return desc;
}

static inline SamplerDesc LinearClampSampler() {
    SamplerDesc desc(FilterMode::LINEAR, FilterMode::LINEAR, AddressMode::CLAMP);
    return desc;
}

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
