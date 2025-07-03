#pragma once
#include "engine/core/base/singleton.h"
#include "engine/core/framework/module.h"

namespace my {

class Scene;

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

#if 0
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btSoftRigidDynamicsWorld;
class btCollisionShape;
class btCollisionObject;
struct btSoftBodyWorldInfo;

namespace my {

class Scene;

struct PhysicsWorldContext {
    // @TODO: free properly
    btDefaultCollisionConfiguration* collisionConfig = nullptr;
    btCollisionDispatcher* dispatcher = nullptr;
    btBroadphaseInterface* broadphase = nullptr;
    btSequentialImpulseConstraintSolver* solver = nullptr;
    btSoftRigidDynamicsWorld* dynamicWorld = nullptr;
    btSoftBodyWorldInfo* softBodyWorldInfo = nullptr;

    std::vector<btCollisionObject*> ghostObjects;
};

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
#endif

}  // namespace my
