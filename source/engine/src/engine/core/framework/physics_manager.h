#pragma once
#include "core/base/singleton.h"
#include "core/framework/event_queue.h"
#include "core/framework/module.h"

class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionShape;

namespace my {

class Scene;

class PhysicsManager : public Singleton<PhysicsManager>, public Module, public EventListener {
public:
    PhysicsManager() : Module("PhysicsManager") {}

    auto Initialize() -> Result<void> override;
    void Finalize() override;

    void Update(Scene& p_scene);

    void EventReceived(std::shared_ptr<Event> p_event) override;

protected:
    void CreateWorld(const Scene& p_scene);
    void CleanWorld();
    bool HasWorld() const { return m_collisionConfig != nullptr; }

    btDefaultCollisionConfiguration* m_collisionConfig = nullptr;
    btCollisionDispatcher* m_dispatcher = nullptr;
    btBroadphaseInterface* m_overlappingPairCache = nullptr;
    btSequentialImpulseConstraintSolver* m_solver = nullptr;
    btDiscreteDynamicsWorld* m_dynamicWorld = nullptr;
    std::vector<btCollisionShape*> m_collisionShapes;
};

}  // namespace my
