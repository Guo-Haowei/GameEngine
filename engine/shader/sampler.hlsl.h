#ifndef SAMPLER_HLSL_H_INCLUDED
#define SAMPLER_HLSL_H_INCLUDED

#ifdef HLSL_LANG
#define SAMPLER_STATE(REG, NAME, DESC) SamplerState NAME : register(s##REG)
#else  // #ifdef HLSL_LANG

#ifndef SAMPLER_STATE
#define SAMPLER_STATE(REG, NAME, DESC)
#endif  // #ifndef SAMPLER_STATE

#endif  // #ifdef HLSL_LANG

SAMPLER_STATE(0, s_linearMipWrapSampler, LinearWrapSampler());
SAMPLER_STATE(1, s_shadowSampler, ShadowMapSampler());
SAMPLER_STATE(2, s_linearClampSampler, LinearClampSampler());
SAMPLER_STATE(3, s_cubemapClampSampler, CubemapSampler());

#endif  // #ifndef SAMPLER_HLSL_H_INCLUDED