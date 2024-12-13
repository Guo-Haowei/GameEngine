#include "game_script.h"

namespace my {

static float Normalize(float p_value, float p_low, float p_high, float p_clamp_low, float p_clamp_high) {
    const float bounded_value = glm::clamp(p_value, p_low, p_high);
    return (bounded_value - p_low) / (p_high - p_low) * (p_clamp_high - p_clamp_low) + p_clamp_low;
}

void PlaneScript::OnCollision(ecs::Entity p_other_id) {
    RigidBodyComponent* rigid_body = m_scene->GetComponent<RigidBodyComponent>(p_other_id);
    DEV_ASSERT(rigid_body);
    switch (rigid_body->collisionType) {
        case COLLISION_BIT_ROCK: {
            TransformComponent* transform_1 = GetComponent<TransformComponent>();
            TransformComponent* transform_2 = m_scene->GetComponent<TransformComponent>(p_other_id);
            Vector3f distance = transform_1->GetWorldMatrix()[3] - transform_2->GetWorldMatrix()[3];
            distance = glm::normalize(distance);
            m_collisionSpeed = 30.0f * Vector2f(distance.x, distance.y);
        } break;
        case COLLISION_BIT_BATTERY: {
            LOG("BATTERY!");
        } break;
        default:
            CRASH_NOW();
            break;
    }
}

void PlaneScript::OnCreate() {
}

void PlaneScript::OnUpdate(float p_timestep) {
    const auto [width, height] = DisplayManager::GetSingleton().GetWindowSize();
    Vector2f mouse = InputManager::GetSingleton().GetCursor();
    mouse.x /= (float)width;
    mouse.y /= (float)height;
    mouse = 2.0f * mouse - 1.0f;
    mouse.y = -mouse.y;

    TransformComponent* transform = GetComponent<TransformComponent>();

    Vector3f translate = transform->GetTranslation();

    m_collisionDisplacement += m_collisionSpeed;

    const float target_x = Normalize(mouse.x, -1.0f, 1.0f, -AMP_WIDTH, -0.7f * AMP_WIDTH) + m_collisionDisplacement.x;
    const float target_y = Normalize(mouse.y, -0.75f, 0.75f, translate.y - AMP_HEIGHT, translate.y + AMP_HEIGHT) + m_collisionDisplacement.y;

    const float speed = 3.0f;
    Vector2f delta(target_x - translate.x, target_y - translate.y);
    delta *= p_timestep * speed;

    translate.x += delta.x;
    translate.y += delta.y;
    translate.y = glm::clamp(translate.y, MIN_HEIGHT, MAX_HEIGHT);

    transform->SetTranslation(translate);

    float rotate_z_angle = 0.3f * delta.y;
    rotate_z_angle = glm::clamp(rotate_z_angle, glm::radians(-60.0f), glm::radians(60.0f));
    Quaternion q(Vector3f(0.0f, 0.0f, rotate_z_angle));
    transform->SetRotation(Vector4f(q.x, q.y, q.z, q.w));

    m_collisionSpeed += -0.2f * m_collisionSpeed;
    m_collisionSpeed = glm::min(m_collisionSpeed, 0.0f);
    m_collisionDisplacement *= 0.9f;
}

void RockScript::OnCollision(ecs::Entity p_other_id) {
    unused(p_other_id);

    TransformComponent* transform = GetComponent<TransformComponent>();
    transform->Translate(Vector3f(0.0f, -1000.0f, 0.0f));
}

void RockScript::OnUpdate(float p_timestep) {
    Vector3f rotation(1, 1, 0);

    TransformComponent* transform = GetComponent<TransformComponent>();
    transform->Rotate(p_timestep * rotation);
}

void BatteryScript::OnCollision(ecs::Entity p_other_id) {
    unused(p_other_id);

    TransformComponent* transform = GetComponent<TransformComponent>();
    transform->Translate(Vector3f(0.0f, -1000.0f, 0.0f));
}

void BatteryScript::OnUpdate(float p_timestep) {
    Vector3f rotation(1, 1, 0);

    TransformComponent* transform = GetComponent<TransformComponent>();
    transform->Rotate(p_timestep * rotation);
}

// @TODO: this should be game start
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

        m_scene->Create<NativeScriptComponent>(rock.id).Bind<RockScript>();
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

        m_scene->Create<NativeScriptComponent>(battery.id).Bind<BatteryScript>();
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
    *mesh = MakeTetrahedronMesh(BATTERY_SIZE);
    mesh->gpuResource = GraphicsManager::GetSingleton().CreateMesh(*mesh);
    DEV_ASSERT(!mesh->subsets.empty());

    ecs::Entity material_id = m_scene->CreateMaterialEntity("battery_material");
    MaterialComponent* material = m_scene->GetComponent<MaterialComponent>(material_id);
    material->baseColor = Vector4f(BLUE_COLOR.r, BLUE_COLOR.g, BLUE_COLOR.b, 1.0f);
    mesh->subsets[0].material_id = material_id;

    m_batteryMesh = mesh_id;
}
}  // namespace my
