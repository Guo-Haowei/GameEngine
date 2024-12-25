#pragma once
#include "engine/scene/scene_component.h"
#include "engine/systems/job_system/job_system.h"

namespace my {

void UpdateMeshEmitter(float p_timestep,
                       const TransformComponent& p_transform,
                       MeshEmitterComponent& p_emitter);

}  // namespace my
