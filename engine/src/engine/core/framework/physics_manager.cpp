#include "physics_manager.h"

#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/scene/scene.h"

WARNING_PUSH()
WARNING_DISABLE(4459, "-Wpedantic")
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
    // CleanWorld();
}

void PhysicsManager::Update(Scene& p_scene) {
    float delta_time = p_scene.m_elapsedTime;
    // delta_time *= 0.1f;

    if (!p_scene.m_physicsWorld) {
        // @TODO: bench mark
        CreateWorld(p_scene);
    }

    PhysicsWorldContext& context = *p_scene.m_physicsWorld;

    context.dynamicWorld->stepSimulation(delta_time, 10);

    for (int j = context.dynamicWorld->getNumCollisionObjects() - 1; j >= 0; j--) {
        btCollisionObject* collision_object = context.dynamicWorld->getCollisionObjectArray()[j];
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
            uint32_t handle = (uint32_t)(uintptr_t)collision_object->getUserPointer();
            ecs::Entity id{ handle };
            if (id.IsValid()) {
                // hack: wind
                for (int node_idx = 0; node_idx < body->m_nodes.size(); ++node_idx) {
                    body->m_nodes[node_idx].m_f = btVector3(0.0f, 0.2f, 0.1f);
                }

                const ClothComponent* cloth = p_scene.GetComponent<ClothComponent>(id);
                cloth->points.clear();
                cloth->normals.clear();

                for (int face_idx = 0; face_idx < body->m_faces.size(); ++face_idx) {
                    const btSoftBody::Face& face = body->m_faces[face_idx];
                    for (int node_idx = 0; node_idx < 3; ++node_idx) {
                        const btSoftBody::Node& node = *face.m_n[node_idx];
                        cloth->points.push_back(Vector3f(node.m_x.getX(), node.m_x.getY(), node.m_x.getZ()));
                        cloth->normals.push_back(Vector3f(node.m_n.getX(), node.m_n.getY(), node.m_n.getZ()));
                    }
                }
            }
            continue;
        }

        CRASH_NOW();
    }
}

void PhysicsManager::CreateWorld(const Scene& p_scene) {
    DEV_ASSERT(!p_scene.m_physicsWorld);

    p_scene.m_physicsWorld = new PhysicsWorldContext;
    PhysicsWorldContext& context = *p_scene.m_physicsWorld;

    context.collisionConfig = new btDefaultCollisionConfiguration();
    context.dispatcher = new btCollisionDispatcher(context.collisionConfig);
    context.broadphase = new btDbvtBroadphase();
    context.solver = new btSequentialImpulseConstraintSolver;
    context.dynamicWorld = new btSoftRigidDynamicsWorld(context.dispatcher, context.broadphase, context.solver, context.collisionConfig);

    btVector3 gravity = btVector3(0.0f, -9.81f, 0.0f);
    context.dynamicWorld->setGravity(gravity);

    context.softBodyWorldInfo = new btSoftBodyWorldInfo;
    context.softBodyWorldInfo->m_broadphase = context.dynamicWorld->getBroadphase();
    context.softBodyWorldInfo->m_dispatcher = context.dynamicWorld->getDispatcher();
    context.softBodyWorldInfo->m_gravity = context.dynamicWorld->getGravity();
    context.softBodyWorldInfo->m_sparsesdf.Initialize();

    btContactSolverInfo& solverInfo = context.dynamicWorld->getSolverInfo();
    solverInfo.m_friction = 0.5f;  // Set appropriate friction

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

        body->setContactProcessingThreshold(0.5f);

        context.dynamicWorld->addRigidBody(body);
    }

    for (auto [id, component] : p_scene.m_ClothComponents) {
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

            cloth->getCollisionShape()->setMargin(0.001f);
            cloth->setUserPointer((void*)(size_t)id.GetId());

            cloth->generateBendingConstraints(2, cloth->appendMaterial());
            cloth->setTotalMass(10.0f);

            cloth->m_cfg.piterations = 5;
            cloth->m_cfg.kDP = 0.005f;

            cloth->setCollisionFlags(btCollisionObject::CO_COLLISION_OBJECT | btCollisionObject::CO_RIGID_BODY | btCollisionObject::CO_SOFT_BODY);
            cloth->setFriction(0.5f);

            MeshComponent mesh;
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

                component.gpuResource = GraphicsManager::GetSingleton().CreateMesh(mesh);
            }

            context.dynamicWorld->addSoftBody(cloth);
        }
    }
}

void PhysicsManager::CleanWorld() {
    // @TODO: delete
}

}  // namespace my
