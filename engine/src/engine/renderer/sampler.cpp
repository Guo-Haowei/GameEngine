#pragma once
#include "sampler.h"

#include "render_graph/render_graph_defines.h"

namespace my {

SamplerDesc CubemapNoMipSampler() {
    SamplerDesc desc(MinFilter::LINEAR,
                     MagFilter::LINEAR,
                     AddressMode::CLAMP,
                     StaticBorderColor::TRANSPARENT_BLACK,
                     0.0f,
                     1,
                     ComparisonFunc::ALWAYS);
    return desc;
}

SamplerDesc CubemapSampler() {
    SamplerDesc desc(MinFilter::LINEAR_MIPMAP_LINEAR,
                     MagFilter::LINEAR,
                     AddressMode::CLAMP,
                     StaticBorderColor::TRANSPARENT_BLACK,
                     0.0f,
                     1,
                     ComparisonFunc::ALWAYS);
    return desc;
}

SamplerDesc CubemapLodSampler() {
    SamplerDesc desc = CubemapSampler();
    desc.maxLod = IBL_MIP_CHAIN_MAX - 1.0f;
    return desc;
}

SamplerDesc ShadowMapSampler() {
    SamplerDesc desc(MinFilter::LINEAR,
                     MagFilter::LINEAR,
                     AddressMode::BORDER, StaticBorderColor::OPAQUE_WHITE);
    return desc;
}

}  // namespace my
