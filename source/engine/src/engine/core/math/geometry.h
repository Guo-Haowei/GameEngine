#pragma once
#include "scene/mesh_component.h"

namespace my {

MeshComponent MakePlaneMesh(const Vector3f& p_scale = Vector3f(0.5f));
MeshComponent MakeCubeMesh(const Vector3f& p_scale = Vector3f(0.5f));

MeshComponent MakeSphereMesh(float p_radius,
                             int p_rings = 60,
                             int p_sectors = 60);
MeshComponent MakeCylinder(float p_radius,
                           float p_height,
                           int p_sectors = 60);
MeshComponent MakeTorus(float p_radius,
                        float p_tube_radius,
                        int p_sectors = 60,
                        int p_tube_sectors = 60);

// @TODO: refactor the following
MeshComponent MakeGrassBillboard(const Vector3f& p_scale = Vector3f(0.5f));
MeshComponent MakeBoxMesh(float p_size = 0.5f);
MeshComponent MakeBoxWireframeMesh(float p_size = 0.5f);

MeshComponent MakeSkyBoxMesh();

}  // namespace my
