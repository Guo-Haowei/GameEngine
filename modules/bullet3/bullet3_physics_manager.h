#pragma once
#include "engine/runtime/physics_manager.h"

namespace my {

class Scene;

class Bullet3PhysicsManager : public IPhysicsManager {
public:
    Bullet3PhysicsManager() : IPhysicsManager("Bullet3PhysicsManager") {}

    void Update(Scene& p_scene) override;

    void OnSimBegin(Scene& p_scene) override;
    void OnSimEnd(Scene& p_scene) override;

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    void UpdateCollision(Scene& p_scene);
    void UpdateSimulation(Scene& p_scene);

    void CreateWorld(Scene& p_scene);
    void CleanWorld();
};

}  // namespace my
