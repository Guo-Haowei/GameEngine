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
static constexpr float MIN_HEIGHT = 15.f;
static constexpr float MAX_HEIGHT = 45.f;
static constexpr float AMP_WIDTH = 30.0f;
static constexpr float AMP_HEIGHT = 32.0f;

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
    void OnCollision(ecs::Entity p_other_id) override;

    void OnCreate() override;

    void OnUpdate(float p_timestep) override;

    Vector2f m_collisionSpeed{ 0.0f };
    Vector2f m_collisionDisplacement{ 0.0f };
};

class RockScript : public ScriptableEntity {
    void OnCollision(ecs::Entity p_other_id) override;

    void OnUpdate(float p_timestep) override;
};

class BatteryScript : public ScriptableEntity {
    void OnCollision(ecs::Entity p_other_id) override;

    void OnUpdate(float p_timestep) override;
};

struct GameObject {
    float lifeRemains;
};

template<typename T, int N>
    requires std::is_base_of_v<GameObject, T>
struct ObjectPool {
    std::array<T, N> pool;
    std::list<T*> deadList;
    std::list<T*> aliveList;

    void Free(T* p_object) {
        deadList.push_back(p_object);
    }

    T* Alloc() {
        if (deadList.empty()) {
            return nullptr;
        }

        T* result = deadList.front();
        deadList.pop_front();
        aliveList.push_back(result);
        return result;
    }

    void Update(float p_timestep, std::function<void(T*)> p_on_destroy) {
        std::list<T*> tmp;
        for (int i = (int)aliveList.size() - 1; i >= 0; --i) {
            T* object = aliveList.back();
            aliveList.pop_back();
            object->lifeRemains -= p_timestep;
            if (object->lifeRemains <= 0.0f) {
                p_on_destroy(object);
                deadList.push_back(object);
            } else {
                tmp.emplace_back(object);
            }
        }
        aliveList = std::move(tmp);
    }
};

class GeneratorScript : public ScriptableEntity {
    enum : uint32_t {
        ROCK_POOL_SIZE = 16,
        BATTERY_POOL_SIZE = 32,
    };

    static constexpr float ROCK_SIZE = 4.0f;
    static constexpr float BATTERY_SIZE = 2.0f;
    static constexpr float ENTITY_LIFE_TIME = 1.5f * glm::pi<float>() / WORLD_SPEED - 3.0f;

    void OnCreate() override;
    void OnUpdate(float p_timestep) override;

    void CreateRockResource();
    void CreateBatteryResource();

    ecs::Entity m_rockMesh;
    ecs::Entity m_batteryMesh;

    float m_time{ 0.0f };
    float m_lastSpawnTime{ 0.0f };

    struct Rock : GameObject {
        ecs::Entity id;
    };

    struct Battery : GameObject {
        ecs::Entity id;
    };

    ObjectPool<Rock, ROCK_POOL_SIZE> m_rockPool;
    ObjectPool<Battery, BATTERY_POOL_SIZE> m_batteryPool;
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
