#pragma once
#include "engine/scene/mesh_component.h"

namespace my {

MeshComponent MakePlaneMesh(const Vector3f& p_scale = Vector3f(0.5f));

MeshComponent MakePlaneMesh(const Vector3f& p_point_0,
                            const Vector3f& p_point_1,
                            const Vector3f& p_point_2,
                            const Vector3f& p_point_3);

MeshComponent MakeCubeMesh(const Vector3f& p_scale = Vector3f(0.5f));

MeshComponent MakeCubeMesh(const std::array<Vector3f, 8>& p_points);

MeshComponent MakeSphereMesh(float p_radius,
                             int p_rings = 60,
                             int p_sectors = 60);

MeshComponent MakeCylinderMesh(float p_radius,
                               float p_height,
                               int p_sectors = 60,
                               int p_height_sector = 1);

MeshComponent MakeConeMesh(float p_radius,
                           float p_height,
                           int p_sectors = 60);

MeshComponent MakeTorusMesh(float p_radius,
                            float p_tube_radius,
                            int p_sectors = 60,
                            int p_tube_sectors = 60);

// @TODO: refactor the following
MeshComponent MakeGrassBillboard(const Vector3f& p_scale = Vector3f(0.5f));
MeshComponent MakeBoxMesh(float p_size = 0.5f);
MeshComponent MakeBoxWireframeMesh(float p_size = 0.5f);

MeshComponent MakeSkyBoxMesh();

}  // namespace my
