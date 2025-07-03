#pragma once
#include "engine/core/base/singleton.h"
#include "engine/core/framework/module.h"

#define USE_BULLET

namespace my {

class Scene;

#if !defined(USE_BULLET)


class PhysicsManager : public Module {
public:
    PhysicsManager() : Module("PhysicsManager") {}

    void Update(Scene&) {}

    virtual void OnSimBegin(Scene&) {}
    virtual void OnSimEnd(Scene&) {}

protected:
    auto InitializeImpl() -> Result<void> override { return Result<void>(); }
    void FinalizeImpl() override {}

    void UpdateCollision(Scene&) {}
    void UpdateSimulation(Scene&) {}

    void CreateWorld(Scene&) {}
    void CleanWorld() {}
};

#else

class PhysicsManager : public Module {
public:
    PhysicsManager() : Module("PhysicsManager") {}

    void Update(Scene& p_scene);

    virtual void OnSimBegin(Scene& p_scene);
    virtual void OnSimEnd(Scene& p_scene);

protected:
    auto InitializeImpl() -> Result<void> override;
    void FinalizeImpl() override;

    void UpdateCollision(Scene& p_scene);
    void UpdateSimulation(Scene& p_scene);

    void CreateWorld(Scene& p_scene);
    void CleanWorld();
};

}  // namespace my

#endif
