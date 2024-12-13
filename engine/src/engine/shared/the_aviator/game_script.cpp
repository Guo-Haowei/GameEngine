#include "game_script.h"

namespace my {

void GeneratorScript::OnCreate() {
    CreateRockResource();
    CreateBatteryResource();

    int counter = 0;
    for (Rock& rock : m_rockPool.pool) {
        rock.lifeRemains = 0.0f;
        rock.id = m_scene->CreateObjectEntity(std::format("rock_{}", ++counter));
        ObjectComponent* object = m_scene->GetComponent<ObjectComponent>(rock.id);
        object->meshId = m_rockMesh;

        m_scene->AttachChild(rock.id, m_id);
        m_rockPool.Free(&rock);

        auto& rigid_body = m_scene->Create<RigidBodyComponent>(rock.id)
                               .InitSphere(0.5f * ROCK_SIZE);
        rigid_body.collisionType = COLLISION_BIT_ROCK;
        rigid_body.collisionMask = COLLISION_BIT_PLAYER;
    }

    counter = 0;
    for (Battery& battery : m_batteryPool.pool) {
        battery.lifeRemains = 0.0f;
        battery.id = m_scene->CreateObjectEntity(std::format("battery_{}", ++counter));
        ObjectComponent* object = m_scene->GetComponent<ObjectComponent>(battery.id);
        object->meshId = m_batteryMesh;

        m_scene->AttachChild(battery.id, m_id);
        m_batteryPool.Free(&battery);

        auto& rigid_body = m_scene->Create<RigidBodyComponent>(battery.id)
                               .InitSphere(0.5f * ROCK_SIZE);
        rigid_body.collisionType = COLLISION_BIT_BATTERY;
        rigid_body.collisionMask = COLLISION_BIT_PLAYER;
    }
}

void GeneratorScript::OnUpdate(float p_timestep) {
    m_time += p_timestep;

    // spawn object
    if (m_time - m_lastSpawnTime > 1.0f) {
        if (Rock* rock = m_rockPool.Alloc(); rock) {
            rock->lifeRemains = ENTITY_LIFE_TIME;
            TransformComponent* transform = m_scene->GetComponent<TransformComponent>(rock->id);
            float angle = m_time * WORLD_SPEED;
            angle += Degree(90.0f).GetRadians();
            const float distance = OCEAN_RADIUS + Random::Float(MIN_HEIGHT, MAX_HEIGHT);
            transform->SetTranslation(distance * Vector3f(glm::sin(angle), glm::cos(angle), 0.0f));
            m_lastSpawnTime = m_time;
        }
        if (Battery* battery = m_batteryPool.Alloc(); battery) {
            battery->lifeRemains = ENTITY_LIFE_TIME;
            TransformComponent* transform = m_scene->GetComponent<TransformComponent>(battery->id);
            float angle = m_time * WORLD_SPEED;
            angle += Degree(80.0f).GetRadians();
            const float distance = OCEAN_RADIUS + Random::Float(MIN_HEIGHT, MAX_HEIGHT);
            transform->SetTranslation(distance * Vector3f(glm::sin(angle), glm::cos(angle), 0.0f));
            m_lastSpawnTime = m_time;
        }
    }

    m_rockPool.Update(p_timestep, [&](Rock* p_rock) {
        TransformComponent* transform = m_scene->GetComponent<TransformComponent>(p_rock->id);
        transform->Translate(Vector3f(0.0f, -1000.0f, 0.0f));
    });
    m_batteryPool.Update(p_timestep, [&](Battery* p_battery) {
        TransformComponent* transform = m_scene->GetComponent<TransformComponent>(p_battery->id);
        transform->Translate(Vector3f(0.0f, -900.0f, 0.0f));
    });
}

void GeneratorScript::CreateRockResource() {
    ecs::Entity mesh_id = m_scene->CreateMeshEntity("rock_mesh");
    MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(mesh_id);
    *mesh = MakeSphereMesh(ROCK_SIZE, 6, 6);
    mesh->gpuResource = GraphicsManager::GetSingleton().CreateMesh(*mesh);
    DEV_ASSERT(!mesh->subsets.empty());

    ecs::Entity material_id = m_scene->CreateMaterialEntity("rock_material");
    MaterialComponent* material = m_scene->GetComponent<MaterialComponent>(material_id);
    material->baseColor = Vector4f(RED_COLOR.r, RED_COLOR.g, RED_COLOR.b, 1.0f);
    mesh->subsets[0].material_id = material_id;

    m_rockMesh = mesh_id;
}

void GeneratorScript::CreateBatteryResource() {
    ecs::Entity mesh_id = m_scene->CreateMeshEntity("battery_mesh");
    MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(mesh_id);
    *mesh = MakeSphereMesh(BATTERY_SIZE, 4, 4);
    mesh->gpuResource = GraphicsManager::GetSingleton().CreateMesh(*mesh);
    DEV_ASSERT(!mesh->subsets.empty());

    ecs::Entity material_id = m_scene->CreateMaterialEntity("battery_material");
    MaterialComponent* material = m_scene->GetComponent<MaterialComponent>(material_id);
    material->baseColor = Vector4f(BLUE_COLOR.r, BLUE_COLOR.g, BLUE_COLOR.b, 1.0f);
    mesh->subsets[0].material_id = material_id;

    m_batteryMesh = mesh_id;
}
}  // namespace my
