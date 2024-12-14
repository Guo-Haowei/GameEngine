#include "physics_manager.h"

#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/scene/scene.h"
#include "engine/scene/scriptable_entity.h"

WARNING_PUSH()
WARNING_DISABLE(4459, "-Wpedantic")
WARNING_DISABLE(4201, "-Wunused-parameter")
#include <BulletCollision/CollisionDispatch/btGhostObject.h>
#include <BulletSoftBody/btSoftBody.h>
#include <BulletSoftBody/btSoftBodyHelpers.h>
#include <BulletSoftBody/btSoftRigidDynamicsWorld.h>
#include <btBulletDynamicsCommon.h>
WARNING_POP()

namespace my {

static btTransform ConvertTransform(const TransformComponent& p_transform) {
    Matrix4x4f world_matrix = p_transform.GetWorldMatrix();
    btTransform transform;
    transform.setIdentity();
    transform.setFromOpenGLMatrix(&world_matrix[0].x);
    return transform;
}

struct CustomContactResultCallback : btCollisionWorld::ContactResultCallback {
    CustomContactResultCallback(Scene& p_scene) : m_scene(p_scene) {
    }

    btScalar addSingleResult(btManifoldPoint&, const btCollisionObjectWrapper* p_wrap_1, int, int, const btCollisionObjectWrapper* p_wrap_2, int, int) override {
        const btCollisionObject* object_1 = p_wrap_1->getCollisionObject();
        const btCollisionObject* object_2 = p_wrap_2->getCollisionObject();

        ecs::Entity entity_1{ (uint32_t)(uintptr_t)object_1->getUserPointer() };
        ecs::Entity entity_2{ (uint32_t)(uintptr_t)object_2->getUserPointer() };

        NativeScriptComponent* script_1 = m_scene.GetComponent<NativeScriptComponent>(entity_1);
        NativeScriptComponent* script_2 = m_scene.GetComponent<NativeScriptComponent>(entity_2);

#if 0
        if (script_1 || script_2) {
            NameComponent* name_1 = m_scene.GetComponent<NameComponent>(entity_1);
            NameComponent* name_2 = m_scene.GetComponent<NameComponent>(entity_2);
            struct A {
                const btCollisionObject* object;
                std::string name;
            };
            std::array<A, 2> objects = { A{ object_1, name_1->GetName() }, A{ object_2, name_2->GetName() } };
            for (const auto& a : objects) {
                const btCollisionObject* obj = a.object;
                auto shape = obj->getCollisionShape();
                btScalar scale;
                btVector3 origin;
                shape->getBoundingSphere(origin, scale);
                origin += obj->getWorldTransform().getOrigin();
                LOG_VERBOSE("{} bounding object radius {}, origin {} {} {}", a.name, scale, origin.getX(), origin.getY(), origin.getZ());
            }
            LOG_OK("(((****");
        }
#endif

        if (script_1 && script_1->instance) {
            script_1->instance->OnCollision(entity_2);
        }

        if (script_2 && script_2->instance) {
            script_2->instance->OnCollision(entity_1);
        }

        return 0.0f;
    }

    Scene& m_scene;
};

class CustomCollisionDispatcher : public btCollisionDispatcher {
public:
    CustomCollisionDispatcher(btCollisionConfiguration* p_config, Scene& p_scene) : btCollisionDispatcher(p_config), m_scene(p_scene) {
    }

    void dispatchAllCollisionPairs(btOverlappingPairCache* pairCache, const btDispatcherInfo& dispatchInfo, btDispatcher* dispatcher) override {
        btCollisionDispatcher::dispatchAllCollisionPairs(pairCache, dispatchInfo, dispatcher);

#if 0
        for (int i = 0; i < getNumManifolds(); ++i) {
            btPersistentManifold* manifold = getManifoldByIndexInternal(i);
            const btCollisionObject* object_1 = manifold->getBody0();
            const btCollisionObject* object_2 = manifold->getBody1();

            ecs::Entity entity_1{ (uint32_t)(uintptr_t)object_1->getUserPointer() };
            ecs::Entity entity_2{ (uint32_t)(uintptr_t)object_2->getUserPointer() };

            NativeScriptComponent* script_1 = m_scene.GetComponent<NativeScriptComponent>(entity_1);
            NativeScriptComponent* script_2 = m_scene.GetComponent<NativeScriptComponent>(entity_2);

            if (script_1 || script_2) {
                NameComponent* name_1 = m_scene.GetComponent<NameComponent>(entity_1);
                NameComponent* name_2 = m_scene.GetComponent<NameComponent>(entity_2);
                struct A {
                    const btCollisionObject* object;
                    std::string name;
                };
                std::array<A, 2> objects = { A{ object_1, name_1->GetName() }, A{ object_2, name_2->GetName() } };
                for (const auto& a : objects) {
                    const btCollisionObject* obj = a.object;
                    auto shape = obj->getCollisionShape();
                    btScalar scale;
                    btVector3 origin;
                    shape->getBoundingSphere(origin, scale);
                    origin += obj->getWorldTransform().getOrigin();
                    auto speed = obj->getInterpolationLinearVelocity();
                    LOG_VERBOSE("{} bounding object radius {}, origin {} {} {}, speed {} {} {}", a.name, scale, origin.getX(), origin.getY(), origin.getZ(), speed.getX(), speed.getY(), speed.getZ());
                }
                LOG_OK("(((****");
            }

            if (script_1 && script_1->instance) {
                script_1->instance->OnCollision(entity_2);
            }
            if (script_2 && script_2->instance) {
                script_2->instance->OnCollision(entity_1);
            }
        }
#endif
    }

    Scene& m_scene;
};

auto PhysicsManager::InitializeImpl() -> Result<void> {
    return Result<void>();
}

void PhysicsManager::FinalizeImpl() {
    // CleanWorld();
    LOG_WARN("PhysicsManager:: unload world properly");
}

void PhysicsManager::UpdateSimulation(Scene& p_scene) {
    const float delta_time = p_scene.m_timestep;

    PhysicsWorldContext& context = *p_scene.m_physicsWorld;

    for (auto object : context.ghostObjects) {
        uint32_t handle = (uint32_t)(uintptr_t)object->getUserPointer();
        ecs::Entity id{ handle };
        DEV_ASSERT(id.IsValid());
        TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
        if (DEV_VERIFY(transform_component)) {
            if (btGhostObject* o = btGhostObject::upcast(object); o) {
                auto transform = ConvertTransform(*transform_component);
                o->setWorldTransform(transform);
            }
        }
    }

    context.dynamicWorld->stepSimulation(delta_time, 10);

    for (int j = context.dynamicWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
        btCollisionObject* collision_object = context.dynamicWorld->getCollisionObjectArray()[j];
        uint32_t handle = (uint32_t)(uintptr_t)collision_object->getUserPointer();
        ecs::Entity id{ handle };
        if (auto body = p_scene.GetComponent<RigidBodyComponent>(id); body && body->objectType == RigidBodyComponent::GHOST) {
            continue;
        }

        if (btRigidBody* body = btRigidBody::upcast(collision_object); body) {
            btTransform transform;

            if (body->getMotionState()) {
                body->getMotionState()->getWorldTransform(transform);
            } else {
                transform = collision_object->getWorldTransform();
            }

            if (id.IsValid()) {
                TransformComponent& transform_component = *p_scene.GetComponent<TransformComponent>(id);
                const btVector3& origin = transform.getOrigin();
                const btQuaternion rotation = transform.getRotation();
                // @TODO: this is wrong, setting local matrix with global matrix
                transform_component.SetTranslation(Vector3f(origin.getX(), origin.getY(), origin.getZ()));
                transform_component.SetRotation(Vector4f(rotation.getX(), rotation.getY(), rotation.getZ(), rotation.getW()));
            }
            continue;
        }

        if (btSoftBody* body = btSoftBody::upcast(collision_object); body) {
            if (id.IsValid()) {
                // hack: wind
                for (int node_idx = 0; node_idx < body->m_nodes.size(); ++node_idx) {
                    body->m_nodes[node_idx].m_f = btVector3(0.0f, 0.2f, 0.1f);
                }

                MeshComponent* mesh = p_scene.GetComponent<MeshComponent>(id);
                DEV_ASSERT(mesh);

                auto& positions = mesh->updatePositions;
                auto& normals = mesh->updateNormals;
                positions.clear();
                normals.clear();

                for (int face_idx = 0; face_idx < body->m_faces.size(); ++face_idx) {
                    const btSoftBody::Face& face = body->m_faces[face_idx];
                    for (int node_idx = 0; node_idx < 3; ++node_idx) {
                        const btSoftBody::Node& node = *face.m_n[node_idx];
                        positions.push_back(Vector3f(node.m_x.getX(), node.m_x.getY(), node.m_x.getZ()));
                        normals.push_back(Vector3f(node.m_n.getX(), node.m_n.getY(), node.m_n.getZ()));
                    }
                }
            }
            continue;
        }

        CRASH_NOW();
    }
}

void PhysicsManager::UpdateCollision(Scene& p_scene) {
    PhysicsWorldContext& context = *p_scene.m_physicsWorld;

    // set positions
    for (int j = context.dynamicWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
        btCollisionObject* collision_object = context.dynamicWorld->getCollisionObjectArray()[j];
        uint32_t handle = (uint32_t)(uintptr_t)collision_object->getUserPointer();
        ecs::Entity id{ handle };
        if (auto body = p_scene.GetComponent<RigidBodyComponent>(id); body && body->objectType == RigidBodyComponent::GHOST) {
            continue;
        }

        TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
        if (!transform_component) {
            continue;
        }

        if (btRigidBody* body = btRigidBody::upcast(collision_object); body) {
            btTransform transform = ConvertTransform(*transform_component);
            body->setWorldTransform(transform);
            body->getMotionState()->setWorldTransform(transform);

            continue;
        }

        if (btSoftBody* body = btSoftBody::upcast(collision_object); body) {
            continue;
        }

        CRASH_NOW();
    }

    context.dynamicWorld->performDiscreteCollisionDetection();

    for (int i = 0; i < context.dynamicWorld->getNumCollisionObjects() - 1; ++i) {
        for (int j = i + 1; j < context.dynamicWorld->getNumCollisionObjects(); ++j) {
            btCollisionObject* object_1 = context.dynamicWorld->getCollisionObjectArray()[i];
            btCollisionObject* object_2 = context.dynamicWorld->getCollisionObjectArray()[j];

            ecs::Entity entity_1{ (uint32_t)(uintptr_t)object_1->getUserPointer() };
            ecs::Entity entity_2{ (uint32_t)(uintptr_t)object_2->getUserPointer() };

            if (!entity_1.IsValid() || !entity_2.IsValid()) {
                continue;
            }

            const RigidBodyComponent* rigid_body_1 = p_scene.GetComponent<RigidBodyComponent>(entity_1);
            const RigidBodyComponent* rigid_body_2 = p_scene.GetComponent<RigidBodyComponent>(entity_2);

            if (!rigid_body_1 || !rigid_body_2) {
                continue;
            }

            const bool check = (rigid_body_1->collisionType & rigid_body_2->collisionMask) ||
                               (rigid_body_2->collisionType & rigid_body_1->collisionMask);

            if (check) {
                CustomContactResultCallback callback(p_scene);
                context.dynamicWorld->contactPairTest(object_1, object_2, callback);
            }
        }
    }
}

void PhysicsManager::Update(Scene& p_scene) {
    CreateWorld(p_scene);

    switch (p_scene.m_physicsMode) {
        case PhysicsMode::SIMULATION:
            UpdateSimulation(p_scene);
            break;
        case PhysicsMode::COLLISION_DETECTION:
            UpdateCollision(p_scene);
            break;
        default:
            break;
    }
}

void PhysicsManager::CreateWorld(Scene& p_scene) {
    if (p_scene.m_physicsMode == PhysicsMode::NONE) {
        return;
    }

    // @TODO: bench mark

    if (!p_scene.m_physicsWorld) {
        p_scene.m_physicsWorld = new PhysicsWorldContext;
        PhysicsWorldContext& context = *p_scene.m_physicsWorld;

        context.collisionConfig = new btDefaultCollisionConfiguration();
        context.dispatcher = new CustomCollisionDispatcher(context.collisionConfig, p_scene);
        context.broadphase = new btDbvtBroadphase();
        context.solver = new btSequentialImpulseConstraintSolver;
        context.dynamicWorld = new btSoftRigidDynamicsWorld(context.dispatcher, context.broadphase, context.solver, context.collisionConfig);

        // btVector3 gravity = btVector3(0, 0, 0);
        btVector3 gravity = btVector3(0.0f, -9.81f, 0.0f);
        context.dynamicWorld->setGravity(gravity);

        context.softBodyWorldInfo = new btSoftBodyWorldInfo;
        context.softBodyWorldInfo->m_broadphase = context.dynamicWorld->getBroadphase();
        context.softBodyWorldInfo->m_dispatcher = context.dynamicWorld->getDispatcher();
        context.softBodyWorldInfo->m_gravity = context.dynamicWorld->getGravity();
        context.softBodyWorldInfo->m_sparsesdf.Initialize();

        btContactSolverInfo& solverInfo = context.dynamicWorld->getSolverInfo();
        solverInfo.m_friction = 0.5f;  // Set appropriate friction
    }

    PhysicsWorldContext& context = *p_scene.m_physicsWorld;

    for (auto [id, component] : p_scene.m_RigidBodyComponents) {
        if (component.physicsObject) {
            continue;
        }

        const TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
        DEV_ASSERT(transform_component);
        if (!transform_component) {
            continue;
        }

        btCollisionShape* shape = nullptr;
        switch (component.shape) {
            case RigidBodyComponent::SHAPE_CUBE: {
                const Vector3f& half = component.param.box.half_size;
                shape = new btBoxShape(btVector3(half.x, half.y, half.z));
                break;
            }
            case RigidBodyComponent::SHAPE_SPHERE: {
                shape = new btSphereShape(component.param.sphere.radius);
                break;
            }
            default:
                CRASH_NOW_MSG("unknown rigidBody.shape");
                break;
        }

        btScalar mass(component.mass);
        bool is_dynamic = (mass != 0.f);  // rigidbody is dynamic if and only if mass is non zero, otherwise static

        btVector3 local_inertia(0, 0, 0);
        if (is_dynamic) {
            shape->calculateLocalInertia(mass, local_inertia);
        }

        btTransform transform = ConvertTransform(*transform_component);
        if (component.objectType == RigidBodyComponent::GHOST) {
            btGhostObject* object = new btGhostObject();
            object->setCollisionShape(shape);
            object->setWorldTransform(transform);
            object->setUserPointer((void*)(size_t)id.GetId());
            context.ghostObjects.push_back(object);
            context.dynamicWorld->addCollisionObject(object);

            component.physicsObject = object;
        } else {
            btDefaultMotionState* motion_state = new btDefaultMotionState(transform);
            btRigidBody::btRigidBodyConstructionInfo info(mass, motion_state, shape, local_inertia);
            btRigidBody* object = new btRigidBody(info);
            object->setUserPointer((void*)(size_t)id.GetId());
            context.dynamicWorld->addRigidBody(object);

            component.physicsObject = object;
        }
    }

    for (auto [id, component] : p_scene.m_ClothComponents) {
        if (component.physicsObject) {
            continue;
        }

        const TransformComponent* transform_component = p_scene.GetComponent<TransformComponent>(id);
        DEV_ASSERT(transform_component);
        if (!transform_component) {
            continue;
        }

        const ObjectComponent* object = p_scene.GetComponent<ObjectComponent>(id);
        if (DEV_VERIFY(object && object->meshId.IsValid())) {
            Vector4f a = transform_component->GetLocalMatrix() * Vector4f(component.point_0, 1.0f);
            Vector4f b = transform_component->GetLocalMatrix() * Vector4f(component.point_1, 1.0f);
            Vector4f c = transform_component->GetLocalMatrix() * Vector4f(component.point_2, 1.0f);
            Vector4f d = transform_component->GetLocalMatrix() * Vector4f(component.point_3, 1.0f);

            btSoftBody* cloth = btSoftBodyHelpers::CreatePatch(*context.softBodyWorldInfo,
                                                               btVector3(a.x, a.y, a.z),
                                                               btVector3(b.x, b.y, b.z),
                                                               btVector3(d.x, d.y, d.z),
                                                               btVector3(c.x, c.y, c.z),
                                                               component.res.x,
                                                               component.res.y,
                                                               component.fixedFlags,
                                                               true);

            cloth->getCollisionShape()->setMargin(0.01f);
            cloth->setUserPointer((void*)(size_t)id.GetId());

            cloth->generateBendingConstraints(2, cloth->appendMaterial());
            cloth->setTotalMass(10.0f);

            cloth->m_cfg.piterations = 5;
            cloth->m_cfg.kDP = 0.005f;

            cloth->setCollisionFlags(btCollisionObject::CO_COLLISION_OBJECT | btCollisionObject::CO_RIGID_BODY | btCollisionObject::CO_SOFT_BODY);
            cloth->setFriction(0.5f);

            MeshComponent& mesh = p_scene.Create<MeshComponent>(id);
            {
                auto& indices = mesh.indices;
                auto& positions = mesh.positions;
                auto& normals = mesh.normals;
                auto& uv = mesh.texcoords_0;

                for (int face_idx = 0; face_idx < cloth->m_faces.size(); ++face_idx) {
                    const btSoftBody::Face& face = cloth->m_faces[face_idx];
                    indices.push_back(3 * face_idx);
                    indices.push_back(3 * face_idx + 1);
                    indices.push_back(3 * face_idx + 2);
                    for (int node_idx = 0; node_idx < 3; ++node_idx) {
                        const btSoftBody::Node& node = *face.m_n[node_idx];
                        positions.emplace_back(Vector3f(node.m_x.getX(), node.m_x.getY(), node.m_x.getZ()));
                        normals.emplace_back(Vector3f(node.m_n.getX(), node.m_n.getY(), node.m_n.getZ()));
                        uv.emplace_back(Vector2f());
                    }
                }

                MeshComponent::MeshSubset subset;
                subset.index_count = static_cast<uint32_t>(mesh.indices.size());
                subset.index_offset = 0;
                mesh.subsets.emplace_back(subset);

                mesh.CreateRenderData();
                mesh.flags |= MeshComponent::DYNAMIC | MeshComponent::DOUBLE_SIDED;
                mesh.gpuResource = GraphicsManager::GetSingleton().CreateMesh(mesh);
            }

            context.dynamicWorld->addSoftBody(cloth);
            component.physicsObject = cloth;
        }
    }
}

void PhysicsManager::CleanWorld() {
    // @TODO: delete
}

}  // namespace my
