#ifndef SAMPLER_HLSL_H_INCLUDED
#define SAMPLER_HLSL_H_INCLUDED

#if defined(__cplusplus)
#else
#endif

SamplerState u_sampler : register(s0);
SamplerState g_shadow_sampler : register(s1);
SamplerState linear_clamp_sampler : register(s2);

#endif