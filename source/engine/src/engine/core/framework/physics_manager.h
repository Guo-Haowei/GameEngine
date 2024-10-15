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

    bool initialize() override;
    void finalize() override;

    void update(Scene& p_scene);

    void eventReceived(std::shared_ptr<Event> p_event) override;

protected:
    void createWorld(const Scene& p_scene);
    void cleanWorld();
    bool hasWorld() const { return m_collision_config != nullptr; }

    btDefaultCollisionConfiguration* m_collision_config = nullptr;
    btCollisionDispatcher* m_dispatcher = nullptr;
    btBroadphaseInterface* m_overlapping_pair_cache = nullptr;
    btSequentialImpulseConstraintSolver* m_solver = nullptr;
    btDiscreteDynamicsWorld* m_dynamic_world = nullptr;
    std::vector<btCollisionShape*> m_collision_shapes;
};

}  // namespace my
