#pragma once

namespace my {

// @TODO: move to appropriate place
enum class AddressMode {
    WRAP,
    CLAMP,
    BORDER,
};

enum class FilterMode {
    NEAREST,
    LINEAR,
    MIPMAP_LINEAR,
};

struct SamplerDesc {
    FilterMode min;
    FilterMode mag;
    AddressMode mode_u = AddressMode::WRAP;
    AddressMode mode_v = AddressMode::WRAP;
    AddressMode mode_w = AddressMode::WRAP;
    float border[4];
};

inline SamplerDesc nearest_sampler() {
    SamplerDesc desc{};
    desc.mode_u = desc.mode_v = desc.mode_w = AddressMode::CLAMP;
    desc.min = desc.mag = FilterMode::NEAREST;
    return desc;
}

inline SamplerDesc linear_clamp_sampler() {
    SamplerDesc desc{};
    desc.min = desc.mag = FilterMode::LINEAR;
    desc.mode_u = desc.mode_v = desc.mode_w = AddressMode::CLAMP;
    return desc;
}

inline SamplerDesc bloom_downsample() {
    SamplerDesc desc{};
    desc.min = desc.mag = FilterMode::LINEAR;
    desc.mode_u = desc.mode_v = desc.mode_w = AddressMode::BORDER;
    desc.border[0] = 0.0;
    desc.border[1] = 0.0;
    desc.border[2] = 0.0;
    desc.border[3] = 1.0f;
    return desc;
}

inline SamplerDesc env_cube_map_sampler_mip() {
    SamplerDesc desc{};
    desc.min = FilterMode::MIPMAP_LINEAR;
    desc.mag = FilterMode::LINEAR;
    desc.mode_u = desc.mode_v = desc.mode_w = AddressMode::CLAMP;
    return desc;
}

inline SamplerDesc shadow_cube_map_sampler() {
    SamplerDesc desc{};
    desc.min = desc.mag = FilterMode::NEAREST;
    desc.mode_u = desc.mode_v = desc.mode_w = AddressMode::CLAMP;
    return desc;
}

inline SamplerDesc shadow_map_sampler() {
    SamplerDesc desc{};
    desc.min = desc.mag = FilterMode::NEAREST;
    desc.mode_u = desc.mode_v = desc.mode_w = AddressMode::BORDER;
    desc.border[0] = 1.0f;
    desc.border[1] = 1.0f;
    desc.border[2] = 1.0f;
    desc.border[3] = 1.0f;
    return desc;
}

}  // namespace my
