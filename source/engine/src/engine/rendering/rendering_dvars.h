#include "core/dynamic_variable/dynamic_variable_begin.h"

// switches
DVAR_BOOL(r_enable_fxaa, DVAR_FLAG_CACHE, "Enalbe fxaa", true);
DVAR_BOOL(r_enable_vxgi, DVAR_FLAG_CACHE, "Enable vxgi", true);
DVAR_BOOL(r_enable_ssao, DVAR_FLAG_CACHE, "Enable ssao", true);
DVAR_BOOL(r_no_texture, DVAR_FLAG_CACHE, "No texture", false);

// GFX
DVAR_STRING(r_backend, 0, "Renderer backend", "opengl");
DVAR_STRING(r_render_graph, 0, "Renderer graph", "vxgi");
DVAR_BOOL(r_gpu_validation, 0, "Enable GPU validation", true);

// voxel GI
DVAR_INT(r_voxel_size, 0, "Voxel size", 64);
DVAR_INT(r_debug_texture, 0, "", 0);
DVAR_FLOAT(r_vxgi_max_world_size, 0, "Maxium voxel size when using vxgi", 40.0f);

// shadow
DVAR_INT(r_shadow_res, 0, "Shadow resolution", 1024 * 2);

// SSAO
DVAR_INT(r_ssaoKernelSize, 0, "", 32);
DVAR_INT(r_ssaoNoiseSize, 0, "", 4);
DVAR_FLOAT(r_ssaoKernelRadius, 0, "", 0.5f);

// c_fxaa_image

#include "core/dynamic_variable/dynamic_variable_end.h"