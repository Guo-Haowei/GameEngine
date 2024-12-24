#pragma once
#include "engine/scene/scene_component.h"

namespace my {

void UpdateMeshEmitter(float p_timestep,
                       const TransformComponent& p_transform,
                       MeshEmitterComponent& p_emitter);

}  // namespace my
