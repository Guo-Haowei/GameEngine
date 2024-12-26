#pragma once
#include "sampler.h"

#include "render_graph/render_graph_defines.h"

namespace my {

SamplerDesc CubemapSampler() {
    SamplerDesc desc(FilterMode::MIPMAP_LINEAR,
                     FilterMode::MIPMAP_LINEAR,
                     AddressMode::WRAP,
                     StaticBorderColor::TRANSPARENT_BLACK,
                     0.0f,
                     1,
                     ComparisonFunc::ALWAYS,
                     0.0f,
                     // IBL_MIP_CHAIN_MAX - 1.0f);
                     0.0f);
    return desc;
}

}  // namespace my
