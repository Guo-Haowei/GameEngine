#include "the_aviator_layer.h"

#include "engine/core/framework/application.h"
#include "engine/scene/scene.h"
#include "the_aviator_script.h"

namespace my {

TheAviatorLayer::TheAviatorLayer() : GameLayer("TheAviator") {
}

void TheAviatorLayer::OnAttachInternal() {
    m_time = 0.0f;
    m_lastSpawnTime = 0.0f;
    m_scene = m_app->GetActiveScene();
    m_id = m_scene->CreateTransformEntity("generator");
    m_rockPool.Clear();
    m_batteryPool.Clear();

    ecs::Entity earth = m_scene->FindEntityByName("earth");
    DEV_ASSERT(earth.IsValid());
    m_scene->AttachChild(m_id, earth);

    auto rock_mesh = m_scene->FindEntityByName("rock_mesh");
    auto battery_mesh = m_scene->FindEntityByName("battery_mesh");

    int counter = 0;
    for (Rock& rock : m_rockPool.pool) {
        rock.lifeRemains = 0.0f;
        rock.id = m_scene->CreateObjectEntity(std::format("rock_{}", ++counter));
        ObjectComponent* object = m_scene->GetComponent<ObjectComponent>(rock.id);
        object->meshId = rock_mesh;

        m_scene->AttachChild(rock.id, m_id);
        m_rockPool.Free(&rock);

        auto& rigid_body = m_scene->Create<RigidBodyComponent>(rock.id)
                               .InitSphere(0.5f * ROCK_SIZE);
        rigid_body.collisionType = COLLISION_BIT_ROCK;
        rigid_body.collisionMask = COLLISION_BIT_PLAYER;

        m_scene->Create<LuaScriptComponent>(rock.id)
            .SetClassName("Rock")
            .SetPath("@res://scripts/rock.lua");
    }

    counter = 0;
    for (Battery& battery : m_batteryPool.pool) {
        battery.lifeRemains = 0.0f;
        battery.id = m_scene->CreateObjectEntity(std::format("battery_{}", ++counter));
        ObjectComponent* object = m_scene->GetComponent<ObjectComponent>(battery.id);
        object->meshId = battery_mesh;

        m_scene->AttachChild(battery.id, m_id);
        m_batteryPool.Free(&battery);

        auto& rigid_body = m_scene->Create<RigidBodyComponent>(battery.id)
                               .InitSphere(0.5f * ROCK_SIZE);
        rigid_body.collisionType = COLLISION_BIT_BATTERY;
        rigid_body.collisionMask = COLLISION_BIT_PLAYER;

        m_scene->Create<LuaScriptComponent>(battery.id)
            .SetClassName("Battery")
            .SetPath("@res://scripts/battery.lua");
    }
}

}
