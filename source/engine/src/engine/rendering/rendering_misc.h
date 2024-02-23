#pragma once
#include "rendering/rendering_dvars.h"

namespace my {

void register_rendering_dvars();

std::array<mat4, 6> cube_map_view_matrices(const vec3& p_eye);

}  // namespace my
