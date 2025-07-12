#include "engine/core/dynamic_variable/dynamic_variable_begin.h"

// window
DVAR_IVEC2(window_resolution, DVAR_FLAG_CACHE, "Request window resolution", 0, 0);

// project
DVAR_STRING(project, DVAR_FLAG_NONE, "Open project at start", "");
DVAR_STRING(scene, DVAR_FLAG_NONE, "Open scene at start", "");

// IO
DVAR_BOOL(verbose, DVAR_FLAG_NONE, "Print verbose log", true);

// gui
DVAR_BOOL(show_editor, DVAR_FLAG_CACHE, "Show editor", true);

#include "engine/core/dynamic_variable/dynamic_variable_end.h"
