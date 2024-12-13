#include "engine/core/base/random.h"
#include "engine/core/framework/display_manager.h"
#include "engine/core/framework/graphics_manager.h"
#include "engine/core/framework/input_manager.h"
#include "engine/core/framework/scene_manager.h"
#include "engine/core/math/color.h"
#include "engine/core/math/geometry.h"
#include "engine/renderer/graphics_dvars.h"
#include "engine/scene/scene.h"
#include "engine/scene/scriptable_entity.h"

namespace std {

template<>
struct hash<my::Vector3f> {
    std::size_t operator()(const my::Vector3f& p_vec) const {
        std::hash<uint8_t> byte_hash;
        size_t result = 0;

        const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(&p_vec);

        // Hash each byte of the memory block
        for (size_t i = 0; i < sizeof(p_vec); ++i) {
            result ^= byte_hash(byte_ptr[i]) + 0x9e3779b9 + (result << 6) + (result >> 2);
        }

        return result;
    }
};

}  // namespace std

namespace my {

static constexpr float OCEAN_RADIUS = 240.0f;
static constexpr int CLOUD_COUNT = 20;
static constexpr float WORLD_SPEED = 0.3f;
static constexpr float ENTITY_LIFE_TIME = 1.5f * glm::pi<float>() / WORLD_SPEED - 3.0f;
static constexpr float MIN_HEIGHT = 15.f;
static constexpr float MAX_HEIGHT = 45.f;
static constexpr float AMP_WIDTH = 30.0f;
static constexpr float AMP_HEIGHT = 32.0f;
static constexpr float ROCK_RADIUS = 4.0f;

static const Color RED_COLOR = Color::Hex(0xCE190A);
static const Color WHITE_COLOR = Color::Hex(0XD8D0D1);
static const Color BROWN_COLOR = Color::Hex(0x59332E);
static const Color DRAK_BROWN_COLOR = Color::Hex(0x23190F);
static const Color BLUE_COLOR = Color::Hex(0X10A8A3);
// static const Color PINK_COLOR = Color::Hex(0xF5986E);

enum : uint32_t {
    COLLISION_BIT_PLAYER = BIT(1),
    COLLISION_BIT_ROCK = BIT(2),
    COLLISION_BIT_BATTERY = BIT(3),
};

static float Normalize(float p_value, float p_low, float p_high, float p_clamp_low, float p_clamp_high) {
    const float bounded_value = glm::clamp(p_value, p_low, p_high);
    return (bounded_value - p_low) / (p_high - p_low) * (p_clamp_high - p_clamp_low) + p_clamp_low;
}

class CameraController : public ScriptableEntity {
protected:
    void OnUpdate(float p_timestep) override {
        PerspectiveCameraComponent* camera = GetComponent<PerspectiveCameraComponent>();
        if (camera) {
            const Vector2f mouse_move = InputManager::GetSingleton().MouseMove();
            if (glm::abs(mouse_move.x) > 2.0f) {
                float angle = camera->GetFovy().GetDegree();
                angle += mouse_move.x * p_timestep * 2.0f;
                angle = glm::clamp(angle, 35.0f, 80.0f);
                camera->SetFovy(Degree(angle));
            }
        }
    }
};

class PlaneScript : public ScriptableEntity {
    void OnCollision(ecs::Entity p_other_id) override {
        NameComponent* name = m_scene->GetComponent<NameComponent>(p_other_id);
        LOG_ERROR("collide with {}", name->GetName());
    }

    void OnCreate() override {
    }

    void OnUpdate(float p_timestep) override {
        const auto [width, height] = DisplayManager::GetSingleton().GetWindowSize();
        Vector2f mouse = InputManager::GetSingleton().GetCursor();
        mouse.x /= (float)width;
        mouse.y /= (float)height;
        mouse = 2.0f * mouse - 1.0f;
        mouse.y = -mouse.y;

        TransformComponent* transform = GetComponent<TransformComponent>();

        Vector3f translate = transform->GetTranslation();

        const float target_x = Normalize(mouse.x, -1.0f, 1.0f, -AMP_WIDTH, -0.7f * AMP_WIDTH);
        const float target_y = Normalize(mouse.y, -0.75f, 0.75f, translate.y - AMP_HEIGHT, translate.y + AMP_HEIGHT);

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
    }
};

class GeneratorScript : public ScriptableEntity {
protected:
    void OnCreate() override {
        CreateRockResource();

        m_rockPool.resize(ROCK_POOL_SIZE);
        int counter = 0;
        for (auto& rock : m_rockPool) {
            rock.lifeRemains = 0.0f;
            rock.id = m_scene->CreateObjectEntity(std::format("rock_{}", ++counter));
            ObjectComponent* object = m_scene->GetComponent<ObjectComponent>(rock.id);
            object->meshId = m_rockMesh;

            m_scene->AttachChild(rock.id, m_id);
            m_rockDeadList.push_back(&rock);

            auto& rigid_body = m_scene->Create<RigidBodyComponent>(rock.id)
                                   .InitSphere(0.5f * ROCK_RADIUS);
            rigid_body.collisionType = COLLISION_BIT_ROCK;
            rigid_body.collisionMask = COLLISION_BIT_PLAYER;

            rigid_body.mass = 1.0f;
        }
    }

    void OnUpdate(float p_timestep) override {
        m_time += p_timestep;

        // spawn object
        if (m_time - m_lastSpawnTime > 1.0f) {
            if (!m_rockDeadList.empty()) {
                auto* rock = m_rockDeadList.front();
                m_rockDeadList.pop_front();
                rock->lifeRemains = ENTITY_LIFE_TIME;

                TransformComponent* transform = m_scene->GetComponent<TransformComponent>(rock->id);
                float angle = m_time * WORLD_SPEED;
                angle += Degree(90.0f).GetRadians();
                const float distance = OCEAN_RADIUS + Random::Float(MIN_HEIGHT, MAX_HEIGHT);
                transform->SetTranslation(distance * Vector3f(glm::sin(angle), glm::cos(angle), 0.0f));
                m_lastSpawnTime = m_time;
                m_rockAliveList.emplace_back(rock);
            }
        }

        std::list<Rock*> tmp;
        for (int i = (int)m_rockAliveList.size() - 1; i >= 0; --i) {
            Rock* rock = m_rockAliveList.back();
            m_rockAliveList.pop_back();
            rock->lifeRemains -= p_timestep;
            if (rock->lifeRemains <= 0.0f) {
                TransformComponent* transform = m_scene->GetComponent<TransformComponent>(rock->id);
                // HACK: move the dead component away
                transform->Translate(Vector3f(0.0f, -1000.0f, 0.0f));
                m_rockDeadList.emplace_back(rock);
            } else {
                tmp.emplace_back(rock);
            }
        }
        m_rockAliveList = std::move(tmp);
    }

    void CreateRockResource() {
        m_rockMesh = m_scene->CreateMeshEntity("rock_mesh");
        MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(m_rockMesh);
        *mesh = MakeSphereMesh(ROCK_RADIUS, 6, 6);
        mesh->gpuResource = GraphicsManager::GetSingleton().CreateMesh(*mesh);
        DEV_ASSERT(!mesh->subsets.empty());
        m_rockMaterial = m_scene->CreateMaterialEntity("rock_material");
        MaterialComponent* material = m_scene->GetComponent<MaterialComponent>(m_rockMaterial);
        material->baseColor = Vector4f(RED_COLOR.r, RED_COLOR.g, RED_COLOR.b, 1.0f);
        mesh->subsets[0].material_id = m_rockMaterial;
    }

private:
    ecs::Entity m_rockMesh;
    ecs::Entity m_rockMaterial;
    ecs::Entity m_tetrahedronMesh;
    float m_time{ 0.0f };
    float m_lastSpawnTime{ 0.0f };

    struct Rock {
        ecs::Entity id;
        float lifeRemains;
    };

    enum : uint32_t {
        ROCK_POOL_SIZE = 32,
    };
    std::vector<Rock> m_rockPool;
    std::list<Rock*> m_rockDeadList;
    std::list<Rock*> m_rockAliveList;
};

class EarthScript : public ScriptableEntity {
    void OnUpdate(float p_timestep) override {
        auto transform = GetComponent<TransformComponent>();
        transform->Rotate(Vector3f(0.0f, 0.0f, p_timestep * WORLD_SPEED));
    }
};

class OceanScript : public ScriptableEntity {
    struct Wave {
        float angle;
        float amp;
        float speed;
    };

    void OnCreate() override {
        const ObjectComponent* object = GetComponent<ObjectComponent>();
        DEV_ASSERT(object);
        const MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(object->meshId);
        DEV_ASSERT(mesh);

        std::unordered_map<Vector3f, Wave> cache;

        for (const Vector3f& position : mesh->positions) {

            auto it = cache.find(position);
            if (it != cache.end()) {
                m_waves.push_back(it->second);
                continue;
            }

            Wave wave;
            wave.angle = Random::Float() * 2.0f * glm::pi<float>();
            wave.amp = 3.5f + Random::Float() * 3.5f;
            wave.speed = 6.f * (Random::Float() * 1.0f);
            m_waves.emplace_back(wave);
            cache[position] = wave;
        }
    }

    void OnUpdate(float p_timestep) override {
        const ObjectComponent* object = GetComponent<ObjectComponent>();
        DEV_ASSERT(object);
        const MeshComponent* mesh = m_scene->GetComponent<MeshComponent>(object->meshId);
        DEV_ASSERT(mesh);

        auto& positions = mesh->updatePositions;
        auto& normals = mesh->updateNormals;
        positions.clear();
        normals.clear();
        positions.reserve(mesh->positions.size());
        normals.reserve(mesh->normals.size());

        for (size_t idx = 0; idx < mesh->positions.size(); idx += 3) {
            Vector3f points[3];
            for (int i = 0; i < 3; ++i) {
                Wave& wave = m_waves[idx + i];
                const Vector3f& position = mesh->positions[idx + i];
                Vector3f new_position = position;
                new_position.x += glm::cos(wave.angle) * wave.amp;
                new_position.z += glm::sin(wave.angle) * wave.amp;
                positions.emplace_back(new_position);
                wave.angle += p_timestep * wave.speed;

                points[i] = new_position;
            }

            Vector3f AB = points[0] - points[1];
            Vector3f AC = points[0] - points[2];
            Vector3f normal = glm::normalize(glm::cross(AB, AC));
            normals.emplace_back(normal);
            normals.emplace_back(normal);
            normals.emplace_back(normal);
        }
    }

    std::vector<Wave> m_waves;
};

}  // namespace my
