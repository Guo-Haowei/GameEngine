#include "physics_manager.h"

#include "engine/core/framework/scene_manager.h"
#include "engine/scene/scene.h"

WARNING_PUSH()
WARNING_DISABLE(4127, "-Wunused-parameter")
WARNING_DISABLE(4100, "-Wunused-parameter")
WARNING_DISABLE(4459, "-Wunused-parameter")  // Hide previous definition
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>
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
    // delta_time *= 0.1f;

    if (HasWorld()) {
        m_dynamicWorld->stepSimulation(delta_time, 10);

        for (int j = m_dynamicWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
            btCollisionObject* collision_object = m_dynamicWorld->getCollisionObjectArray()[j];
            if (btRigidBody* body = btRigidBody::upcast(collision_object); body) {
                btTransform transform;

                if (body && body->getMotionState()) {
                    body->getMotionState()->getWorldTransform(transform);
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
                continue;
            }
            if (btSoftBody* body = btSoftBody::upcast(collision_object); body) {
                if (body) {
                    uint32_t handle = (uint32_t)(uintptr_t)collision_object->getUserPointer();
                    ecs::Entity id{ handle };
                    if (id.IsValid()) {
                        const SoftBodyComponent* soft_body = p_scene.GetComponent<SoftBodyComponent>(id);

                        soft_body->faces.clear();
                        soft_body->points.clear();
                        soft_body->normals.clear();

                        for (int face_idx = 0; face_idx < body->m_faces.size(); ++face_idx) {
                            const btSoftBody::Face& face = body->m_faces[face_idx];
                            soft_body->faces.push_back(3 * face_idx);
                            soft_body->faces.push_back(3 * face_idx + 1);
                            soft_body->faces.push_back(3 * face_idx + 2);
                            for (int node_idx = 0; node_idx < 3; ++node_idx) {
                                const btSoftBody::Node& node = *face.m_n[node_idx];
                                soft_body->points.push_back(Vector3f(node.m_x.getX(), node.m_x.getY(), node.m_x.getZ()));
                                soft_body->normals.push_back(Vector3f(node.m_n.getX(), node.m_n.getY(), node.m_n.getZ()));
                            }
                        }
#if 0
                        for (int face_idx = 0; face_idx < body->m_faces.size(); ++face_idx) {
                            const btSoftBody::Face& face = body->m_faces[face_idx];
                            soft_body->faces.push_back(face.m_n[0]->index);
                            soft_body->faces.push_back(face.m_n[1]->index);
                            soft_body->faces.push_back(face.m_n[2]->index);
                        }
                        for (int node_idx = 0; node_idx < body->m_nodes.size(); ++node_idx) {
                            const btSoftBody::Node& node = body->m_nodes[node_idx];
                            soft_body->points.push_back(Vector3f(node.m_x.getX(), node.m_x.getY(), node.m_x.getZ()));
                            soft_body->normals.push_back(Vector3f(node.m_n.getX(), node.m_n.getY(), node.m_n.getZ()));
                        }
#endif
                    }
                }
                continue;
            }
        }
    }
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
    m_softBodyWorldInfo->m_broadphase = m_dynamicWorld->getBroadphase();
    m_softBodyWorldInfo->m_dispatcher = m_dynamicWorld->getDispatcher();
    m_softBodyWorldInfo->m_gravity = m_dynamicWorld->getGravity();
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
        bool is_dynamic = (mass != 0.f);  // rigidbody is dynamic if and only if mass is non zero, otherwise static

        btVector3 local_inertia(0, 0, 0);
        if (is_dynamic) {
            shape->calculateLocalInertia(mass, local_inertia);
        }

        // using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
        btDefaultMotionState* motion_state = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo info(mass, motion_state, shape, local_inertia);
        btRigidBody* body = new btRigidBody(info);
        body->setUserPointer((void*)(size_t)id.GetId());
        m_dynamicWorld->addRigidBody(body);
    }

    for (auto [id, component] : p_scene.m_SoftBodyComponents) {
        const TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
        DEV_ASSERT(transform_component);
        if (!transform_component) {
            continue;
        }

        const ObjectComponent* object = p_scene.GetComponent<ObjectComponent>(id);
        if (DEV_VERIFY(object && object->meshId.IsValid())) {
            const int h = 3;
            btVector3 p0(4, h, 0);
            btVector3 p1(-4, h, 0);
            btVector3 p2(-4, h, -8);
            btVector3 p3(4, h, -8);
            btSoftBody* cloth = btSoftBodyHelpers::CreatePatch(*m_softBodyWorldInfo,
                                                               p0, p1, p2, p3,
                                                               4, 4, 3, true);

            cloth->getCollisionShape()->setMargin(0.001f);
            cloth->setUserPointer((void*)(size_t)id.GetId());

            cloth->generateBendingConstraints(2, cloth->appendMaterial());
            cloth->setTotalMass(10.0f);

            cloth->m_cfg.piterations = 5;
            cloth->m_cfg.kDP = 0.005f;

            m_dynamicWorld->addSoftBody(cloth);
        }
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
