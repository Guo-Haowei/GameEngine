#pragma once

// clang-format off
namespace my { class Scene; }
namespace my::jobsystem { class Context; }
// clang-format on

namespace my {

class Scene;

void RunLightUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep);
void RunTransformationUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep);
void RunHierarchyUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep);
void RunAnimationUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep);
void RunArmatureUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep);
void RunObjectUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep);
void RunParticleEmitterUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep);
void RunMeshEmitterUpdateSystem(Scene& p_scene, jobsystem::Context& p_context, float p_timestep);

}  // namespace my
