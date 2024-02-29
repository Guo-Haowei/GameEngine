#pragma once
#include "assets/image.h"
#include "rendering/sampler.h"
#include "rendering/texture.h"
#include "scene/scene.h"

namespace my {
using PointShadowHandle = int;
constexpr PointShadowHandle INVALID_POINT_SHADOW_HANDLE = -1;
}  // namespace my

namespace my::renderer {

bool initialize();
void finalize();

bool need_update_env();
void reset_need_update_env();
void request_env_map(const std::string& path);

void fill_constant_buffers(const Scene& scene);

void register_rendering_dvars();

std::array<mat4, 6> cube_map_view_matrices(const vec3& p_eye);

void fill_texture_and_sampler_desc(const Image* p_image, TextureDesc& p_texture_desc, SamplerDesc& p_sampler_desc);

PointShadowHandle allocate_point_light_shadow_map();
void free_point_light_shadow_map(PointShadowHandle& p_handle);

}  // namespace my::renderer
