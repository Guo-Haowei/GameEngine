#pragma once
#include "scene/scene_components.h"

namespace my {

MeshComponent make_plane_mesh(float size = 0.5f);
MeshComponent make_box_mesh(float size = 0.5f);
MeshComponent make_box_wireframe_mesh(float size = 0.5f);

}  // namespace my
