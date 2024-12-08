#include "physics_manager.h"

#include "engine/core/framework/scene_manager.h"
#include "engine/scene/scene.h"

WARNING_PUSH()
WARNING_DISABLE(4127, "-Wunused-parameter")
WARNING_DISABLE(4100, "-Wunused-parameter")
WARNING_DISABLE(4459, "-Wunused-parameter")  // Hide previous definition
#include <bullet3/BulletSoftBody/btSoftBody.h>
#include <bullet3/BulletSoftBody/btSoftBodyHelpers.h>
#include <bullet3/BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <bullet3/btBulletDynamicsCommon.h>
WARNING_POP()

namespace my {

auto PhysicsManager::InitializeImpl() -> Result<void> {
    return Result<void>();
}

void PhysicsManager::FinalizeImpl() {
    CleanWorld();
}

void PhysicsManager::EventReceived(std::shared_ptr<IEvent> p_event) {
    SceneChangeEvent* e = dynamic_cast<SceneChangeEvent*>(p_event.get());
    if (!e) {
        return;
    }

    const Scene& scene = *e->GetScene();
    // @TODO: fix
    CreateWorld(scene);
}

void PhysicsManager::Update(Scene& p_scene) {
    float delta_time = p_scene.m_elapsedTime;

    if (HasWorld()) {
        m_dynamicWorld->stepSimulation(delta_time, 10);

        for (int j = m_dynamicWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
            btCollisionObject* collision_object = m_dynamicWorld->getCollisionObjectArray()[j];
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
                transform_component.SetTranslation(Vector3f(origin.getX(), origin.getY(), origin.getZ()));
                transform_component.SetRotation(Vector4f(rotation.getX(), rotation.getY(), rotation.getZ(), rotation.getW()));
            }
        }
    }
}

btSoftBody* CreateSoftBodyFromPoints(btSoftBodyWorldInfo* softBodyWorldInfo, const std::vector<btVector3>& points) {
    // Create a soft body
    btSoftBody* softBody = new btSoftBody(softBodyWorldInfo);

    // Add nodes (points) to the soft body
    for (const auto& point : points) {
        softBody->appendNode(point, 1.0f);  // Append nodes to the soft body
    }

    // Create springs between the points to form the structure of the soft body
    for (int i = 0; i < (int)points.size(); ++i) {
        for (int j = i + 1; j < (int)points.size(); ++j) {
            softBody->appendLink(i, j);  // Create a spring between nodes i and j
        }
    }

    softBody->m_nodes;
#if 0
    // Apply material properties (optional)
    btSoftBody::Material* material = softBody->appendMaterial();
    material->m_kLST = 0.1f;  // Stretching stiffness
    material->m_kAST = 0.1f;  // Bending stiffness
#endif

    softBody->generateBendingConstraints(2);  // Define the bending constraint for the soft body

    return softBody;
}

void PhysicsManager::CreateWorld(const Scene& p_scene) {
    m_collisionConfig = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collisionConfig);
    m_broadphase = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver;
    m_dynamicWorld = new btSoftRigidDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collisionConfig);

    btVector3 gravity = btVector3(0.0f, -9.81f, 0.0f);
    m_dynamicWorld->setGravity(gravity);

    m_softBodyWorldInfo = new btSoftBodyWorldInfo;
    m_softBodyWorldInfo->m_broadphase = m_broadphase;
    m_softBodyWorldInfo->m_dispatcher = m_dispatcher;
    m_softBodyWorldInfo->m_gravity = gravity;
    m_softBodyWorldInfo->m_sparsesdf.Initialize();

    for (auto [id, rigid_body] : p_scene.m_RigidBodyComponents) {
        const TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
        DEV_ASSERT(transform_component);
        if (!transform_component) {
            continue;
        }

        btCollisionShape* shape = nullptr;
        switch (rigid_body.shape) {
            case RigidBodyComponent::SHAPE_CUBE: {
                const Vector3f& half = rigid_body.param.box.half_size;
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

        m_collisionShapes.push_back(shape);

        const Vector3f& origin = transform_component->GetTranslation();
        const Vector4f& rotation = transform_component->GetRotation();
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
        m_dynamicWorld->addRigidBody(body);
    }
    for (auto [id, component] : p_scene.m_SoftBodyComponents) {
        btSoftBody* soft_body = new btSoftBody(m_softBodyWorldInfo);
        m_dynamicWorld->addSoftBody(soft_body);
    }
}

void PhysicsManager::CleanWorld() {
    if (HasWorld()) {
        // remove the rigidbodies from the dynamics world and delete them
        for (int i = m_dynamicWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
            btCollisionObject* obj = m_dynamicWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState()) {
                delete body->getMotionState();
            }
            m_dynamicWorld->removeCollisionObject(obj);
            delete obj;
        }

        // delete collision shapes
        for (size_t j = 0; j < m_collisionShapes.size(); j++) {
            btCollisionShape* shape = m_collisionShapes[j];
            m_collisionShapes[j] = 0;
            delete shape;
        }

        delete m_softBodyWorldInfo;
        m_softBodyWorldInfo = nullptr;

        delete m_dynamicWorld;
        m_dynamicWorld = nullptr;

        delete m_solver;
        m_solver = nullptr;

        delete m_broadphase;
        m_broadphase = nullptr;

        delete m_dispatcher;
        m_dispatcher = nullptr;

        delete m_collisionConfig;
        m_collisionConfig = nullptr;

        m_collisionShapes.clear();
    }
}

}  // namespace my
