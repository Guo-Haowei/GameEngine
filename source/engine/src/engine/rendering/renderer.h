#pragma once
#include "scene/scene.h"

namespace my::renderer {

bool need_update_env();
void reset_need_update_env();
void request_env_map(const std::string& path);

void fill_constant_buffers(const Scene& scene);

void register_rendering_dvars();

std::array<mat4, 6> cube_map_view_matrices(const vec3& p_eye);

}  // namespace my::renderer
