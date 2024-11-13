#ifndef SAMPLER_HLSL_H_INCLUDED
#define SAMPLER_HLSL_H_INCLUDED

#if defined(__cplusplus)
#ifndef SAMPLER_STATE
#define SAMPLER_STATE(...)
#endif

#elif defined(HLSL_LANG)
#define SAMPLER_STATE(REG, NAME, DESC) SamplerState NAME : register(s##REG)
#endif

SAMPLER_STATE(0, s_linearMipWrapSampler,
              SamplerDesc(FilterMode::MIPMAP_LINEAR,
                          FilterMode::MIPMAP_LINEAR,
                          AddressMode::WRAP));
// @TODO: may be change this to LINEAR
SAMPLER_STATE(1, s_shadowSampler,
              SamplerDesc(FilterMode::LINEAR,
                          FilterMode::LINEAR,
                          AddressMode::BORDER,
                          StaticBorderColor::OPAQUE_BLACK));
SAMPLER_STATE(2, s_linearClampSampler,
              SamplerDesc(FilterMode::MIPMAP_LINEAR,
                          FilterMode::MIPMAP_LINEAR,
                          AddressMode::CLAMP));

#endif