#include "engine/core/dynamic_variable/dynamic_variable_begin.h"

DVAR_IVEC2(resolution, DVAR_FLAG_NONE, "Frame resolution", 1920, 1080);

// @TODO: dvar range

// General
#if USING(PLATFORM_APPLE)
DVAR_STRING(gfx_backend, DVAR_FLAG_NONE, "Renderer backend", "metal");
#else
DVAR_STRING(gfx_backend, DVAR_FLAG_NONE, "Renderer backend", "opengl");
#endif
DVAR_STRING(gfx_render_graph, DVAR_FLAG_NONE, "Renderer graph", "scene3d");
DVAR_BOOL(gfx_gpu_validation, DVAR_FLAG_NONE, "Enable GPU validation", true);

// Switches
DVAR_BOOL(gfx_debug_shadow, DVAR_FLAG_CACHE, "Debug shadow", false);
DVAR_BOOL(gfx_enable_bloom, DVAR_FLAG_CACHE, "Enable Bloom", true);
DVAR_BOOL(gfx_enable_ibl, DVAR_FLAG_CACHE, "Enable IBL", false);

// SSAO
DVAR_BOOL(gfx_ssao_enabled, DVAR_FLAG_CACHE, "Enable SSAO", true);
DVAR_FLOAT(gfx_ssao_radius, DVAR_FLAG_CACHE, "SSAO Radius", 0.5f);

// voxel GI
DVAR_BOOL(gfx_debug_vxgi, DVAR_FLAG_NONE, "Debug VXGI", false);
DVAR_INT(gfx_debug_vxgi_voxel, DVAR_FLAG_NONE, "Select which voxel texture to display", 0);
DVAR_INT(gfx_voxel_size, DVAR_FLAG_NONE, "Voxel size", 64);

// path tracer
DVAR_BOOL(gfx_bvh_generate, DVAR_FLAG_NONE, "Generate BVH", false);
DVAR_INT(gfx_bvh_debug, DVAR_FLAG_NONE, "Debug BVH level", -1);

// shadow
DVAR_INT(gfx_point_shadow_res, DVAR_FLAG_NONE, "Point shadow resolution", 1024);
DVAR_INT(gfx_shadow_res, DVAR_FLAG_NONE, "Shadow resolution", 1024 * 2);

// Bloom
DVAR_FLOAT(gfx_bloom_threshold, DVAR_FLAG_NONE, "", 1.3f);

#include "engine/core/dynamic_variable/dynamic_variable_end.h"