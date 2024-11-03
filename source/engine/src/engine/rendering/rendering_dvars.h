#include "core/dynamic_variable/dynamic_variable_begin.h"

DVAR_IVEC2(resolution, 0, "Frame resolution", 1920, 1080);

// Debug switch
DVAR_BOOL(gfx_debug_shadow, DVAR_FLAG_CACHE, "Debug shadow", false);

//@TODO: rename to gfx_
DVAR_BOOL(r_enable_vxgi, DVAR_FLAG_CACHE, "Enable VXGI", true);
DVAR_BOOL(r_no_texture, DVAR_FLAG_CACHE, "No texture", false);
DVAR_BOOL(r_debug_vxgi, DVAR_FLAG_CACHE, "Debug VXGI", false);

// GFX
DVAR_STRING(r_backend, 0, "Renderer backend", "opengl");
DVAR_STRING(r_render_graph, 0, "Renderer graph", "vxgi");
DVAR_BOOL(r_gpu_validation, 0, "Enable GPU validation", true);

// voxel GI
DVAR_INT(r_voxel_size, 0, "Voxel size", 128);
DVAR_FLOAT(r_vxgi_max_world_size, 0, "Maxium voxel size when using vxgi", 40.0f);
DVAR_INT(r_debug_vxgi_voxel, DVAR_FLAG_CACHE, "", 0);

// shadow
DVAR_INT(r_point_shadow_res, 0, "Point shadow resolution", 1024);
DVAR_INT(r_shadow_res, 0, "Shadow resolution", 1024 * 4);

// Bloom
DVAR_BOOL(r_enable_bloom, DVAR_FLAG_NONE, "Enable Bloom", true);
DVAR_INT(r_debug_bloom_downsample, DVAR_FLAG_NONE, "", 0);
DVAR_FLOAT(r_bloom_threshold, DVAR_FLAG_NONE, "", 1.3f);

#include "core/dynamic_variable/dynamic_variable_end.h"