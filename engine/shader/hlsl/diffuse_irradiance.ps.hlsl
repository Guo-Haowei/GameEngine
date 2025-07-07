/// File: diffuse_irradiance.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

TextureCube t_Skybox : register(t0);

// include implementation
#include "shared_diffuse_irradiance.h"
