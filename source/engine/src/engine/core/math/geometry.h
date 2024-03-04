#pragma once
#include "scene/mesh_component.h"

namespace my {

MeshComponent make_plane_mesh(const vec3& p_scale = vec3(0.5f));
MeshComponent make_cube_mesh(const vec3& p_scale = vec3(0.5f));
MeshComponent make_sphere_mesh(float p_radius = 0.5f, int p_rings = 60, int p_sectors = 60);
MeshComponent make_grass_billboard(const vec3& p_scale = vec3(0.5f));

// @TODO: refactor the following
MeshComponent make_box_mesh(float size = 0.5f);
MeshComponent make_box_wireframe_mesh(float size = 0.5f);

MeshComponent make_sky_box_mesh();

}  // namespace my
