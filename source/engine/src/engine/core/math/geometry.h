#pragma once
#include "scene/mesh_component.h"

namespace my {

MeshComponent MakePlaneMesh(const vec3& p_scale = vec3(0.5f));
MeshComponent MakeCubeMesh(const vec3& p_scale = vec3(0.5f));
MeshComponent MakeSphereMesh(float p_radius = 0.5f, int p_rings = 60, int p_sectors = 60);
MeshComponent MakeCylinder(float p_radius = 0.5f, float p_height = 1.0f, int p_sectors = 60);
MeshComponent MakeGrassBillboard(const vec3& p_scale = vec3(0.5f));

// @TODO: refactor the following
MeshComponent MakeBoxMesh(float p_size = 0.5f);
MeshComponent MakeBoxWireframeMesh(float p_size = 0.5f);

MeshComponent MakeSkyBoxMesh();

}  // namespace my
