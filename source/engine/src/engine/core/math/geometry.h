#pragma once
#include "scene/mesh_component.h"

namespace my {

MeshComponent makePlaneMesh(const vec3& p_scale = vec3(0.5f));
MeshComponent makeCubeMesh(const vec3& p_scale = vec3(0.5f));
MeshComponent makeSphereMesh(float p_radius = 0.5f, int p_rings = 60, int p_sectors = 60);
MeshComponent makeGrassBillboard(const vec3& p_scale = vec3(0.5f));

// @TODO: refactor the following
MeshComponent makeBoxMesh(float p_size = 0.5f);
MeshComponent makeBoxWireframeMesh(float p_size = 0.5f);

MeshComponent makeSkyBoxMesh();

}  // namespace my
