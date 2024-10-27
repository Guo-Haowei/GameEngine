#include "physics_manager.h"

#include "core/framework/scene_manager.h"
#include "scene/scene.h"

#pragma warning(push)
#pragma warning(disable : 4127)
#include "bullet3/btBulletDynamicsCommon.h"
#pragma warning(pop)

namespace my {

bool PhysicsManager::initialize() {
    return true;
}

void PhysicsManager::finalize() {
    cleanWorld();
}

void PhysicsManager::eventReceived(std::shared_ptr<Event> p_event) {
    SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(p_event.get());
    if (!e) {
        return;
    }

    const Scene& scene = *e->getScene();
    // @TODO: fix
    createWorld(scene);
}

void PhysicsManager::update(Scene& p_scene) {
    float delta_time = p_scene.m_delta_time;

    if (hasWorld()) {
        m_dynamic_world->stepSimulation(delta_time, 10);

        for (int j = m_dynamic_world->getNumCollisionObjects() - 1; j >= 0; j--) {
            btCollisionObject* collision_object = m_dynamic_world->getCollisionObjectArray()[j];
            btRigidBody* rigid_body = btRigidBody::upcast(collision_object);
            btTransform transform;

            if (rigid_body && rigid_body->getMotionState()) {
                rigid_body->getMotionState()->getWorldTransform(transform);
            } else {
                transform = collision_object->getWorldTransform();
            }

            uint32_t handle = (uint32_t)(uintptr_t)collision_object->getUserPointer();
            ecs::Entity id{ handle };
            if (id.IsValid()) {
                TransformComponent& transform_component = *p_scene.GetComponent<TransformComponent>(id);
                const btVector3& origin = transform.getOrigin();
                const btQuaternion rotation = transform.getRotation();
                transform_component.SetTranslation(vec3(origin.getX(), origin.getY(), origin.getZ()));
                transform_component.SetRotation(vec4(rotation.getX(), rotation.getY(), rotation.getZ(), rotation.getW()));
            }
        }
    }
}

void PhysicsManager::createWorld(const Scene& p_scene) {
    m_collision_config = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collision_config);
    m_overlapping_pair_cache = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver;
    m_dynamic_world = new btDiscreteDynamicsWorld(m_dispatcher, m_overlapping_pair_cache, m_solver, m_collision_config);

    m_dynamic_world->setGravity(btVector3(0, -10, 0));

    for (auto [id, rigid_body] : p_scene.m_RigidBodyComponents) {
        const TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
        DEV_ASSERT(transform_component);
        if (!transform_component) {
            continue;
        }

        btCollisionShape* shape = nullptr;
        switch (rigid_body.shape) {
            case RigidBodyComponent::SHAPE_CUBE: {
                const vec3& half = rigid_body.param.box.half_size;
                shape = new btBoxShape(btVector3(half.x, half.y, half.z));
                break;
            }
            case RigidBodyComponent::SHAPE_SPHERE: {
                shape = new btSphereShape(rigid_body.param.sphere.radius);
                break;
            }
            default:
                CRASH_NOW_MSG("unknown rigidBody.shape");
                break;
        }

        m_collision_shapes.push_back(shape);

        const vec3& origin = transform_component->GetTranslation();
        const vec4& rotation = transform_component->GetRotation();
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(origin.x, origin.y, origin.z));
        transform.setRotation(btQuaternion(rotation.x, rotation.y, rotation.z, rotation.w));

        btScalar mass(rigid_body.mass);
        bool isDynamic = (mass != 0.f);  // rigidbody is dynamic if and only if mass is non zero, otherwise static

        btVector3 localInertia(0, 0, 0);
        if (isDynamic) {
            shape->calculateLocalInertia(mass, localInertia);
        }

        // using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
        btDefaultMotionState* myMotionState = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, shape, localInertia);
        btRigidBody* body = new btRigidBody(rbInfo);
        body->setUserPointer((void*)((size_t)id.GetId()));
        m_dynamic_world->addRigidBody(body);
    }
}

void PhysicsManager::cleanWorld() {
    if (hasWorld()) {
        // remove the rigidbodies from the dynamics world and delete them
        for (int i = m_dynamic_world->getNumCollisionObjects() - 1; i >= 0; i--) {
            btCollisionObject* obj = m_dynamic_world->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState()) {
                delete body->getMotionState();
            }
            m_dynamic_world->removeCollisionObject(obj);
            delete obj;
        }

        // delete collision shapes
        for (int j = 0; j < m_collision_shapes.size(); j++) {
            btCollisionShape* shape = m_collision_shapes[j];
            m_collision_shapes[j] = 0;
            delete shape;
        }

        // delete dynamics world
        delete m_dynamic_world;
        m_dynamic_world = nullptr;

        // delete m_solver
        delete m_solver;
        m_solver = nullptr;

        // delete broadphase
        delete m_overlapping_pair_cache;
        m_overlapping_pair_cache = nullptr;

        // delete m_dispatcher
        delete m_dispatcher;
        m_dispatcher = nullptr;

        delete m_collision_config;
        m_collision_config = nullptr;

        m_collision_shapes.clear();
    }
}

}  // namespace my
