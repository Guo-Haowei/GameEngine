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

                        soft_body->points.clear();
                        soft_body->points.reserve(body->m_nodes.size());
                        soft_body->normals.clear();
                        soft_body->normals.reserve(body->m_nodes.size());

                        for (int idx = 0; idx < body->m_nodes.size(); ++idx) {
                            const auto& point = body->m_nodes[idx];
                            soft_body->points.push_back(Vector3f(point.m_x.getX(), point.m_x.getY(), point.m_x.getZ()));
                            soft_body->normals.push_back(Vector3f(point.m_n.getX(), point.m_n.getY(), point.m_n.getZ()));
                        }

                        DEV_ASSERT(soft_body);
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
        bool isDynamic = (mass != 0.f);  // rigidbody is dynamic if and only if mass is non zero, otherwise static

        btVector3 localInertia(0, 0, 0);
        if (isDynamic) {
            shape->calculateLocalInertia(mass, localInertia);
        }

        // using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
        btDefaultMotionState* motion_state = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo info(mass, motion_state, shape, localInertia);
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
            const MeshComponent* mesh = p_scene.GetComponent<MeshComponent>(object->meshId);

            std::vector<btScalar> points;
            std::vector<int> indices;

            for (const Vector3f& position : mesh->positions) {
                Vector4f point = transform_component->GetWorldMatrix() * Vector4f(position, 1.0f);
                points.push_back(point.x);
                points.push_back(point.y);
                points.push_back(point.z);
                points.push_back(1.0f);
            }

            for (int idx = 0; idx < (int)mesh->indices.size(); idx += 3) {
                uint32_t index_0 = mesh->indices[idx];
                uint32_t index_1 = mesh->indices[idx + 1];
                uint32_t index_2 = mesh->indices[idx + 2];
                indices.push_back(index_0);
                indices.push_back(index_1);
                indices.push_back(index_2);
            }

            btSoftBody* soft_body = btSoftBodyHelpers::CreateFromTriMesh(*m_softBodyWorldInfo,
                                                                         points.data(),
                                                                         indices.data(),
                                                                         (int)indices.size() / 3);

            // auto flags = soft_body->getCollisionFlags();
            // soft_body->setCollisionFlags(flags | btCollisionObject::CF_NO_CONTACT_RESPONSE);

            btSoftBody::Material* material = soft_body->appendMaterial();
            material->m_kLST = 0.1f;  // Stretching stiffness
            material->m_kAST = 0.1f;  // Bending stiffness

            soft_body->generateBendingConstraints(2, material);
            soft_body->setTotalMass(1.0f, true);
            soft_body->randomizeConstraints();

            soft_body->setUserPointer((void*)(size_t)id.GetId());

            m_dynamicWorld->addSoftBody(soft_body);
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
