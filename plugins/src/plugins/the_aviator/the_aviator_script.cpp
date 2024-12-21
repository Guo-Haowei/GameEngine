#include "the_aviator_script.h"

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
            auto distance = transform_1->GetWorldMatrix()[3] - transform_2->GetWorldMatrix()[3];
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
    Quaternion q(glm::vec3(0.0f, 0.0f, rotate_z_angle));
    transform->SetRotation(Vector4f(q.x, q.y, q.z, q.w));

    m_collisionSpeed += -0.2f * m_collisionSpeed;
    m_collisionSpeed = math::min(m_collisionSpeed, Vector2f::Zero);
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

}  // namespace my
