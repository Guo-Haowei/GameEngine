#include "engine/core/dynamic_variable/dynamic_variable_begin.h"

DVAR_IVEC2(resolution, 0, "Frame resolution", 1920, 1080);

// General
#if USING(PLATFORM_APPLE)
DVAR_STRING(gfx_backend, 0, "Renderer backend", "metal");
#else
DVAR_STRING(gfx_backend, 0, "Renderer backend", "opengl");
#endif
DVAR_STRING(gfx_render_graph, 0, "Renderer graph", "default");
DVAR_BOOL(gfx_gpu_validation, 0, "Enable GPU validation", true);

// Switches
DVAR_BOOL(gfx_debug_shadow, DVAR_FLAG_CACHE, "Debug shadow", false);
DVAR_BOOL(gfx_enable_vxgi, DVAR_FLAG_CACHE, "Enable VXGI", true);
DVAR_BOOL(gfx_no_texture, DVAR_FLAG_CACHE, "No texture", false);
DVAR_BOOL(gfx_debug_vxgi, DVAR_FLAG_CACHE, "Debug VXGI", false);
DVAR_BOOL(gfx_enable_bloom, DVAR_FLAG_CACHE, "Enable Bloom", true);

// voxel GI
DVAR_INT(gfx_voxel_size, 0, "Voxel size", 64);
DVAR_FLOAT(gfx_vxgi_max_world_size, 0, "Maxium voxel size when using vxgi", 40.0f);
DVAR_INT(gfx_debug_vxgi_voxel, DVAR_FLAG_CACHE, "", 0);

// shadow
DVAR_INT(gfx_point_shadow_res, 0, "Point shadow resolution", 1024);
DVAR_INT(gfx_shadow_res, 0, "Shadow resolution", 1024);

// Bloom
DVAR_FLOAT(gfx_bloom_threshold, DVAR_FLAG_NONE, "", 1.3f);

#include "engine/core/dynamic_variable/dynamic_variable_end.h"